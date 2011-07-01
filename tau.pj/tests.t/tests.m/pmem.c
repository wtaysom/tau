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
int	Max;

bool init_entry (int i, char *name, u64 value)
{
	mem_s	*m = &Mem[i];

	strncpy(m->name, name, MAX_NAME);
	m->old_value = m->new_value = value;
	return TRUE;
}

void init_table (const char *path)
{
	Max = number_of_entries(path);
	Mem = ezalloc(Max * sizeof(mem_s));
	scan_file(path, init_entry);
}

bool tick_entry (int i, char *name, u64 value)
{
	mem_s	*m;

	if (i < 0)  fatal("%d  for %s=%lld", i, name , value);
	if (i >= Max) {
		fatal("Bad index %d >= %d for %s=%lld",
				i, Max, name , value);
	}
	m = &Mem[i];
	if (strncmp(m->name, name, MAX_NAME) != 0) {
		fatal("Bad name %s expected %s", name, m->name);
	}
	m->old_value = m->new_value;
	m->new_value = value;
	return TRUE;
}

void tick (const char *path)
{
	scan_file(path, tick_entry);
}

void display (void)
{
	int	i;
	mem_s	*m;

	for (i = 0; i < Max; i++) {
		m = &Mem[i];
		printf("%-20s %lld\n", m->name, m->new_value - m->old_value);
	}
}

int main (int argc, char *argv[])
{
	const char	path[] = "/proc/meminfo";

	init_table(path);
	for (;;) {
		tick(path);
		display();
		usleep(1000000);
	}
	return 0;
}
