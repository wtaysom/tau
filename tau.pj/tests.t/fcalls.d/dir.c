/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Test directory operations
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <debug.h>
#include <eprintf.h>
#include <mystdlib.h>
#include <puny.h>

#include "fcalls.h"
#include "util.h"

enum {	T_NIL  = 0,
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

static bool ignore (char *name)
{
	return (strcmp(name, ".") == 0) ||
		(strcmp(name, "..") == 0);
}

static inline bool CheckEmpty(dir_s *d) { return d->next == 0; }

static void AddFile (dir_s *d, int type, char *name)
{
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

static file_s *FindFile (dir_s *d, char *name)
{
	int i;

	for (i = 0; i < d->next; i++) {
		if (strcmp(d->file[i].name, name) == 0) {
			return &d->file[i];
		}
	}
	return NULL;
}

static void RemoveFile (dir_s *d, char *name)
{
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

static void AddChildren (dir_s *dir, int num_children)
{
	char *name;
	int i;

	chdir(dir->name);
	for (i = 0; i < num_children; i++) {
		name = RndName();
		mkdir(name, 0777);
		AddFile(dir, T_DIR, name);
	}
	chdir("..");
}

static void DeleteChildren (dir_s *dir)
{
	chdir(dir->name);
	while (!CheckEmpty(dir)) {
		rmdir(dir->file[0].name);
		RemoveFile(dir, dir->file[0].name);
	}
	chdir("..");
}

void for_each_file (dir_s *dir, dir_f fn, void *user)
{
	DIR *d;
	struct dirent entry;
	struct dirent *result;
	d = opendir(dir->name);
	for (;;) {
		readdir_r(d, &entry, &result);
		if (result == NULL) break;
		if (!fn(&entry, user)) break;
	}
	closedir(d);
}

static bool AuditChild (struct dirent *entry, void *user)
{
	dir_s *dir = user;
	if (ignore(entry->d_name)) return TRUE;
	if (!FindFile(dir, entry->d_name)) {
		PrError("Didn't find %s in %s", entry->d_name, dir->name);
		return FALSE;
	}
	return TRUE;
}

static void AuditDir (dir_s *dir)
{
	for_each_file(dir, AuditChild, dir);
}

static void SaveTell (DIR *d, struct dirent *entry, dir_s *dir)
{
	file_s *f;
	if (ignore(entry->d_name)) return;
	if ((f = FindFile(dir, entry->d_name)) == NULL) {
		PrError("Didn't find %s in %s", entry->d_name, dir->name);
		return;
	}
	f->tell = telldir(d);
}

void SeekTest (void)
{
	int num_children = 10;
	dir_s dir = NilDir;
	DIR *d;
	struct dirent *entry;
	int i;

	dir.name = RndName();
	mkdir(dir.name, 0777);
	AddChildren(&dir, num_children);

	d = opendir(dir.name);
	while ((entry = readdir(d)) != NULL) {
		SaveTell(d, entry, &dir);
	}
	for (i = 0; i < 27; i++) {
		int k = urand(num_children);
		file_s *file = &dir.file[k];
		seekdir(d, file->tell);
		entry = readdir(d);
		if (strcmp(entry->d_name, file->name) != 0) {
			PrError("Seekdir failed. Looking for %s found %s",
				entry->d_name, file->name);
		}
	}
	closedir(d);
	DeleteChildren(&dir);
	rmdir(dir.name);
	free(dir.name);
}

enum { NAME_SIZE = 17 };

typedef struct DirEntry_s DirEntry_s;
struct DirEntry_s {
	int type;
	char *name;
	DirEntry_s *sibling;
	DirEntry_s *child;
};

typedef bool (*walk_f)(DirEntry_s *dir);

static DirEntry_s *CreateDirEntry (int type)
{
	int fd;
	DirEntry_s *direntry;
	direntry = ezalloc(sizeof(*direntry));
	direntry->name = RndName();
	direntry->type = type;
	switch (type) {
		case T_FILE:
			fd = creat(direntry->name, 0700);
			close(fd);
			break;
		case T_DIR:
			mkdir(direntry->name, 0700);
			break;
		default:
			PrError("bad type %d", type);
			break;
	}
	return direntry;
}

static void Indent (int indent)
{
	while (indent--) {
		printf("  ");
	}
}

static void PrintTree (DirEntry_s *direntry, int indent)
{
	if (!direntry) return;
	if (Option.print) {
		Indent(indent);
		printf("%s %s\n", direntry->type == T_DIR ? "D" : "F",
			direntry->name);
	}
	PrintTree(direntry->child, indent + 1);
	PrintTree(direntry->sibling, indent);
}

static bool WalkTree (DirEntry_s *direntry, walk_f f)
{
	bool continue_walking;
	if (!direntry) return TRUE;
	if (!WalkTree(direntry->sibling, f)) return FALSE;
	if (direntry->type == T_DIR) {
		chdir(direntry->name);
		continue_walking = WalkTree(direntry->child, f);
		chdir("..");
		if (!continue_walking) return FALSE;
	}
	return f(direntry);
}

static bool DeleteDirEntry (DirEntry_s *direntry)
{
	switch (direntry->type) {
		case T_FILE:
			unlink(direntry->name);
			break;
		case T_DIR:
			rmdir(direntry->name);
			break;
		default:
			PrError("bad type %d", direntry->type);
			break;
	}
	return TRUE;
}

static void DeleteTree (DirEntry_s *direntry)
{
	WalkTree(direntry, DeleteDirEntry);
}

static DirEntry_s *CreateTree (int width, int depth, int level)
{
	int i;
	DirEntry_s *direntry;
	DirEntry_s *child;
	int type = random_percent(level * 15) ? T_FILE : T_DIR;
	direntry = CreateDirEntry(type);
	if ((depth == 0) || (type != T_DIR)) {
		return direntry;
	}
	chdir(direntry->name);
	for (i = 0; i < width; i++) {
		child = CreateTree(width, depth - 1, level + 1);
		child->sibling = direntry->child;
		direntry->child = child;
	}
	chdir("..");
	return direntry;
}

static void TreeTest (void)
{
	DirEntry_s *root;
	root = CreateTree(4, 9, 0);
	PrintTree(root, 0);
	DeleteTree(root);
}

static void Simple (void)
{
	int num_children = 20;
	dir_s dir = NilDir;

	dir.name = RndName();
	mkdir(dir.name, 0777);
	AddChildren(&dir, num_children);
	AuditDir(&dir);
	DeleteChildren(&dir);
	rmdir(dir.name);
	free(dir.name);
}

void DirTest (void)
{
	Simple();
	TreeTest();
//  SeekTest();
}
