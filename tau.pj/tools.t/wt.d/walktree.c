/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */
#define _XOPEN_SOURCE 500
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <eprintf.h>
#include <myio.h>
#include <qsort.h>
#include <timer.h>

enum { NUM_BUCKETS = 65 };

typedef struct Histogram_s {
	unint	bucket[NUM_BUCKETS];
	u64	num;
	u64	sum;
} Histogram_s;

typedef struct Ftw_s {
	u64	files;
	u64	dirs;
	u64	dir_no_read;
	u64	no_stat;
	u64	dir_path;
	u64	symlink;
	u64	broken_symlink;
} Ftw_s;

typedef struct Stat_s {
	u64	gt1link;
	u64	reg;
	u64	special;
} Stat_s;

Ftw_s Ftw;
Stat_s Stat;
Histogram_s Histogram;

static int highbit(u64 val)
{
	int i;

	for (i = 0; val; val >>= 1, i++) {}
	return i;
}

void histogram_record(Histogram_s *h, u64 val)
{
	++h->bucket[highbit(val)];
	++h->num;
	h->sum += val;
}

void pr_histogram(Histogram_s *h)
{
	int i;
	int last;
	bool found = FALSE;
	u64 total = 0;
	u64 median = 0;

	for (i = 0; i < NUM_BUCKETS; i++) {
		if (h->bucket[i]) {
			total += h->bucket[i];
			last = i;
		}
	}
	for (i = 0; i <= last; i++) {
		median += h->bucket[i];
		if (median >= total / 2) {
			if (!found) printf("---median---\n");
			median = 0;
		}
		printf("%2d. %5ld\n", i, h->bucket[i]);
	}
	printf("Number %lld average %lld\n", h->num, h->sum / h->num);
}

void pr_ftw_flag(const char *dir)
{
	printf("%s:\n"
		"\tfiles=%lld dirs=%lld dir_no_read=%lld no_stat=%lld"
		" dir_path=%lld symlink=%lld broken_symlink=%lld total=%lld\n",
		dir,
		Ftw.files, Ftw.dirs, Ftw.dir_no_read, Ftw.no_stat,
		Ftw.dir_path, Ftw.symlink, Ftw.broken_symlink,
		Ftw.files + Ftw.dirs + Ftw.dir_no_read + Ftw.no_stat
		+ Ftw.dir_path + Ftw.symlink + Ftw.broken_symlink);
}

void pr_Stat(void)
{
	printf("hardlinks > 1 = %lld regular=%lld special=%lld\n",
		Stat.gt1link, Stat.reg, Stat.special);
}

typedef struct info_s info_s;
struct info_s {
	info_s	*s_next;
	ino_t	s_ino;
	dev_t	s_dev;
	off_t	s_size;
	char	s_name[];
};

enum { HASH_BUCKETS = 98317 };

info_s *Bucket[HASH_BUCKETS];
unint NumInfo;

void pr_info(char *label, info_s *s)
{
	printf("%s ino=%6ld dev=%5ld size=%8ld %s\n",
		label,
		s->s_ino, s->s_dev, s->s_size, s->s_name);
}

info_s *hash(ino_t ino, dev_t dev)
{
	unint h = ino * dev % HASH_BUCKETS;

	return (info_s *)&Bucket[h];
}

info_s *find(ino_t ino, dev_t dev)
{
	info_s *s = hash(ino, dev);

	for (;;) {
		s = s->s_next;
		if (!s) return NULL;
		if ((s->s_ino == ino) && (s->s_dev == dev)) {
			return s;
		}
	}
}

void add(const char *fpath, const struct stat *sb)
{
	info_s *s;
	info_s *t;

	s = find(sb->st_ino, sb->st_dev);
	if (s) return;  // should count this to confirm number of hardlinks

	s = emalloc(sizeof(*s) + strlen(fpath) + 1);

	strcpy(s->s_name, fpath);
	s->s_ino  = sb->st_ino;
	s->s_dev  = sb->st_dev;
	s->s_size = sb->st_size;

	t = hash(s->s_ino, s->s_dev);
	s->s_next = t->s_next;
	t->s_next = s;

	++NumInfo;
}

int info_cmp(const void *a, const void *b)
{
	const info_s *r = a;
	const info_s *s = b;

	if (r->s_size > s->s_size) return 1;
	if (r->s_size == s->s_size) return 0;
	return -1;
}

void find_median(void)
{
	void **set;
	void **next;
	info_s *s;
	u64 avg = (Histogram.sum + Histogram.num / 2) / Histogram.num;
	int i;

	next = set = emalloc(NumInfo * sizeof(info_s *));
	for (i = 0; i < HASH_BUCKETS; i++) {
		s = Bucket[i];
		while (s) {
			*next++ = s;
			s = s->s_next;
		}
	}
	quickSort(set, NumInfo, info_cmp);
	for (i = 0; i < NumInfo; i++) {
		s = set[i];
		if (s->s_size > avg) {
			printf("%d. n=%ld avg=%lld", i, NumInfo, avg);
			pr_info("Average File: ", s);
			break;
		}
	}
	pr_info("Median File: ", set[NumInfo / 2]);
	
	free(set);
}		

int do_entry(
	const char *fpath,
	const struct stat *sb,
	int typeflag,
	struct FTW *ftwbuf)
{
	static dev_t current = 0;

	if (sb->st_dev < 100) {
		/*
		 * Ignore special system file systems
		 * /sys /proc /tmp ...
		 */
		return 0;
	}

//	printf("%s\n", fpath);
	if (sb->st_dev != current) {
		current = sb->st_dev;
		printf("%4ld %s\n", current, fpath);
	}
	switch (typeflag) {
	case FTW_F:
		++Ftw.files;
		if (sb->st_nlink > 2) {
			++Stat.gt1link;
			//printf("%s\n", fpath);
		}
		if (S_ISREG(sb->st_mode)) {
			++Stat.reg;
			histogram_record(&Histogram, sb->st_size);
			if (sb->st_size == 0) {
				//printf("%s\n", fpath);
			}
			add(fpath, sb);
		} else {
			++Stat.special;
		}
		break;
	case FTW_D:
		++Ftw.dirs;
		break;
	case FTW_DNR:
		++Ftw.dir_no_read;
		break;
	case FTW_NS:
		++Ftw.no_stat;
		break;
	case FTW_DP:
		++Ftw.dir_path;
		break;
	case FTW_SL:
		++Ftw.symlink;
		break;
	case FTW_SLN:
		++Ftw.broken_symlink;
		break;
	default:
		warn("Don't know file type %s=%d", fpath, typeflag);
		break;
	}
	return 0;
}

void nftw_walk_tree(char *dir)
{
	nftw(dir, do_entry, 200, FTW_CHDIR | FTW_PHYS /*| FTW_MOUNT*/);
}

int main (int argc, char *argv[])
{
	char *dir = ".";
	tick_t start;
	tick_t finish;
	tick_t total;

	if (argc > 1) dir = argv[1];

	start = nsecs();
	nftw_walk_tree(dir);
	finish = nsecs();
	total = finish - start;
	printf("%lld nsecs %g secs\n", total, (double)total/1000000000.0);

	pr_ftw_flag(dir);
	pr_Stat();
	pr_histogram(&Histogram);
	find_median();
	return 0;
}
