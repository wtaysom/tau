#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <ctype.h>

#include <style.h>
#include <eprintf.h>

typedef void (*dir_f)(struct dirent *d);

void scan (char *name, dir_f fn)
{
	DIR		*dir;
	struct dirent	*d;

	dir = opendir(name);
	if (!dir) fatal("opendir %s:", name);
	for (;;) {
		d = readdir(dir);
		if (!d) break;
		fn(d);
	}
}

void pr_dir (struct dirent *d)
{
#ifdef __APPLE__
	printf("%10llx %10llx %2x %s\n",
		(u64)d->d_ino, (u64)d->d_seekoff, d->d_type, d->d_name);
#else
	printf("%10llx %10llx %2x %s\n",
		(u64)d->d_ino, (u64)d->d_off, d->d_type, d->d_name);
#endif
}

bool no_white_space (char *name)
{
	for (; *name; name++) {
		if (isspace(*name)) return FALSE;
	}
	return TRUE;
}

void _ (struct dirent *d)
{
	char	*name = d->d_name;
	char	new[1024];
	char	*r;
	char	*s;
	int	rc;

	pr_dir(d);
	if (no_white_space(name)) return;

	for (r = name, s = new; *r; r++, s++) {
	if (isspace(*r)) {
			*s = '_';
		} else {
			*s = *r;
		}
	}
	*s = '\0';

	rc = rename(name, new);
	if (rc) fatal("rename %s to %s:", name, new);
}

void usage (void)
{
	printf("Replaces white space with _ in file names\n"
		"Usage: %s { <directory> }+\n", getprogname() );
}

int main (int argc, char *argv[])
{
	int	i;

	setprogname(argv[0]);
	if (argc < 2) usage();
	for (i = 1; i < argc; i++) {
		scan(argv[i], _);
	}
	return 0;
}
