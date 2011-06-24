/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <style.h>
#include <eprintf.h>
#include <debug.h>

enum { MAX_NAME = 24 };

typedef struct mem_s {
	char	name[MAX_NAME];
	u64	old_value;
	u64	new_value;
} mem_s;

typedef bool (*scanner_f)(int i, char *name, u64 value);

int scan_file (const char *path, scanner_f scanner)
{
	char	name[MAX_NAME];
	u64	value;
	FILE	*f;
	int	rc;
	int	i;

	f = fopen(path, "r");
	if (!f) {
		fatal("fopen %s:", path);
	}
	for (i = 0;;i++) {
		rc = fscanf(f, "%30s %llu%*[^\n]\n", name, &value);
		if (rc == EOF) {
			break;
		}
		if (!scanner(i, name, value)) {
			break;
		}
	}
	fclose(f);
	return i;
}

bool count_entry (int i, char *name, u64 value)
{
	return TRUE;
}

int number_of_entries (const char *path)
{
	return scan_file(path, count_entry);
}

mem_s	*Mem;

bool init_entry (int i, char *name, u64 value)
{
	mem_s	*m = &Mem[i];

	strncpy(m->name, name, MAX_NAME);
	m->old_value = m->new_value = value;
	return TRUE;
}

void init_table (const char *path)
{
	int	n = number_of_entries(path);

	Mem = ezalloc(n * sizeof(mem_s));
	scan_file(path, init_entry);
}

int main (int argc, char *argv[])
{
	const char	path[] = "/proc/meminfo";
	const char	pattern[] = "MemFree:";
	char	name[MAX_NAME];
	u64	value;
	u64	old = 0;
	FILE	*f;
	int	k;

	PRd(number_of_entries(path));
	for (;;) {
		f = fopen(path, "r");
		if (!f) {
			fatal("fopen %s:", path);
		}
		for (;;) {
			k = fscanf(f, "%30s %llu%*[^\n]\n", name, &value);
			if (k == EOF) {
				fatal("didn't find %s", pattern);
			}
			if (strncmp(pattern, name, 30) == 0) {
				printf("%lld\n", value - old);
				old = value;
				break;
			}
		}
		fclose(f);
		usleep(1000000);
	}
	return 0;
}
