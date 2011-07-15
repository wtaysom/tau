/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

/* Test directory operations
 */
#include <stdlib.h>
#include <string.h>

#include <eprintf.h>

#include "debug.h"
#include "fcalls.h"
#include "util.h"

enum { T_NIL  = 0,
  T_FILE = 1,
  T_DIR  = 2 };

typedef struct file_s {
  int type;
  char *name;
} file_s;

typedef struct dir_s {
  int max;
  int next;
  file_s *file;
} dir_s;

static file_s NilFile = { 0, NULL };
static dir_s NilDir  = { 0, 0, NULL };

static bool ignore (char *name) {
  return (strcmp(name, ".") == 0) ||
      (strcmp(name, "..") == 0);	
}

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

static int FindFile (dir_s *d, char *name) {
  int i;

  for (i = 0; i < d->next; i++) {
    if (strcmp(d->file[i].name, name) == 0) {
      return d->file[i].type;
    }
  }
  return T_NIL;
}

static void RmvFile (dir_s *d, char *name) {
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

// void for_each_file (dir_s *d, void (*file_fn)(dir_s *))

static void AddChildren (dir_s *d, int num_children) {
  char *name;
  int i;

  for (i = 0; i < num_children; i++) {
    name = RndName(10);
    mkdir(name, 0777);
    AddFile(d, T_DIR, name);
  }
}

static void Simple (void) {
  int num_children = 20;
  char *parent = RndName(10);
  DIR *dir;
  struct dirent *ent;
  dir_s d = NilDir;

  mkdir(parent, 0777);
  chdir(parent);
  AddChildren(&d, num_children);
  dir = opendir(".");
  for (;;) {
    ent = readdir(dir);
    if (!ent) break;
    if (ignore(ent->d_name)) continue;
    if (!FindFile(&d, ent->d_name)) {
      PrError("Didn't find %s", ent->d_name);
    }
  }
  closedir(dir);
  while (d.next) {
    rmdir(d.file[0].name);
    RmvFile(&d, d.file[0].name);
  }
  chdir("..");
  rmdir(parent);
  free(parent);
}

void DirTest (void) {
  Simple();
}
