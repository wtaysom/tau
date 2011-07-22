/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Test directory operations
 */
#include <stdlib.h>
#include <string.h>

#include <debug.h>
#include <eprintf.h>
#include <mystdlib.h>

#include "fcalls.h"
#include "util.h"

enum { T_NIL  = 0,
  T_FILE = 1,
  T_DIR  = 2 };

typedef struct file_s {
  int type;
  char *name;
  long tell;
} file_s;

typedef struct dir_s {
  char *name;
  int max;       /* Max number of slots for files */
  int next;      /* Next file slot */
  file_s *file;  /* Slots for files that are in directory */
} dir_s;

typedef bool (*dir_f)(struct dirent *entry, void *user);

static file_s NilFile = { 0, NULL, 0 };
static dir_s NilDir  = { NULL, 0, 0, NULL };

static bool ignore (char *name) {
  return (strcmp(name, ".") == 0) ||
      (strcmp(name, "..") == 0);
}

static inline bool IsEmpty(dir_s *d) { return d->next == 0; }

static void AddFile (dir_s *d, int type, char *name) {
  if (d->max == d->next) {
    int more = (d->max + 1) * 2;
    file_s *f = erealloc(d->file, more * sizeof(file_s));
    d->max = more;
    d->file = f;
  }
  d->file[d->next].type = type;
  d->file[d->next].name = name;
  ++d->next;
}

static file_s *FindFile (dir_s *d, char *name) {
  int i;

  for (i = 0; i < d->next; i++) {
    if (strcmp(d->file[i].name, name) == 0) {
      return &d->file[i];
    }
  }
  return NULL;
}

static void RemoveFile (dir_s *d, char *name) {
  int i;

  for (i = 0; i < d->next; i++) {
    if (strcmp(d->file[i].name, name) == 0) {
      free(d->file[i].name);
      --d->next;
      d->file[i] = d->file[d->next];
      d->file[d->next] = NilFile;
      return;
    }
  }
}

static void AddChildren (dir_s *parent, int num_children) {
  char *name;
  int i;

  chdir(parent->name);
  for (i = 0; i < num_children; i++) {
    name = RndName(10);
    mkdir(name, 0777);
    AddFile(parent, T_DIR, name);
  }
  chdir("..");
}

static void DeleteChildren (dir_s *parent) {
  chdir(parent->name);
  while (!IsEmpty(parent)) {
    rmdir(parent->file[0].name);
    RemoveFile(parent, parent->file[0].name);
  }
  chdir("..");
}

void for_each_file (dir_s *parent, dir_f fn, void *user) {
  DIR *dir;
  struct dirent entry;
  struct dirent *result;
  dir = opendir(parent->name);
  for (;;) {
    readdir_r(dir, &entry, &result);
    if (result == NULL) break;
    if (!fn(&entry, user)) break;
  }
  closedir(dir);
}

static bool AuditChild (struct dirent *entry, void *user) {
  dir_s *d = user;
  if (ignore(entry->d_name)) return TRUE;
  if (!FindFile(d, entry->d_name)) {
    PrError("Didn't find %s in %s", entry->d_name, d->name);
    return FALSE;
  }
  return TRUE;
}

static void AuditDir (dir_s *d) {
  for_each_file(d, AuditChild, d);
}

static void Simple (void) {
  int num_children = 20;
  dir_s parent = NilDir;

  parent.name = RndName(10);
  mkdir(parent.name, 0777);
  AddChildren(&parent, num_children);
  AuditDir(&parent);
  DeleteChildren(&parent);
  rmdir(parent.name);
  free(parent.name);
}

static void SaveTell (DIR *dir, struct dirent *entry, dir_s *parent) {
  file_s *f;
  if (ignore(entry->d_name)) return;
  if ((f = FindFile(parent, entry->d_name)) == NULL) {
    PrError("Didn't find %s in %s", entry->d_name, parent->name);
    return;
  }
  f->tell = telldir(dir);
}

void SeekTest (void) {
  int num_children = 10;
  dir_s parent = NilDir;
  DIR *dir;
  struct dirent *entry;
  int i;

  parent.name = RndName(14);
  mkdir(parent.name, 0777);
  AddChildren(&parent, num_children);

  dir = opendir(parent.name);
  while ((entry = readdir(dir)) != NULL) {
    SaveTell(dir, entry, &parent);
  }
  for (i = 0; i < 27; i++) {
    int k = urand(num_children);
    file_s *file = &parent.file[k];
    seekdir(dir, file->tell);
    entry = readdir(dir);
    if (strcmp(entry->d_name, file->name) != 0) {
      PrError("Seekdir failed. Looking for %s found %s",
          entry->d_name, file->name);
    }
  }
  closedir(dir);
  DeleteChildren(&parent);
  rmdir(parent.name);
  free(parent.name);
}

void DirTest (void) {
  Simple();
}
