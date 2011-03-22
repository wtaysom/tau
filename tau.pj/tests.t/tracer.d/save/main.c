
#define _XOPEN_SOURCE 500

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <debug.h>
#include <style.h>

#define _STR(x) #x
#define STR(x) _STR(x)
#define MAX_PATH 256

const char *find_debugfs(void)
{
       static char debugfs[MAX_PATH+1];
       static int debugfs_found;
       char type[100];
       FILE *fp;

       if (debugfs_found)
	       return debugfs;

       if ((fp = fopen("/proc/mounts","r")) == NULL) {
	       perror("/proc/mounts");
	       return NULL;
       }

       while (fscanf(fp, "%*s %"
		     STR(MAX_PATH)
		     "s %99s %*s %*d %*d\n",
		     debugfs, type) == 2) {
	       if (strcmp(type, "debugfs") == 0)
		       break;
       }
       fclose(fp);

       if (strcmp(type, "debugfs") != 0) {
	       fprintf(stderr, "debugfs not mounted");
	       return NULL;
       }

       strcat(debugfs, "/tracing/");
       debugfs_found = 1;

       return debugfs;
}

const char *tracing_file(const char *file_name)
{
       static char trace_file[MAX_PATH+1];
       snprintf(trace_file, MAX_PATH, "%s/%s", find_debugfs(), file_name);
       return trace_file;
}

typedef struct sys_enter_format_s sys_enter_format_s;
struct sys_enter_format_s {
	u32	pid;
	u32	lock_depth;
	u8	preempt_count;
	u8	flags;
	u16	type;
	unint	id;
	unint	args[6];
	u64	nsecs;
} __attribute__((packed));

typedef struct trace_s trace_s;
struct trace_s {
	u32	pid;
	u32	id;
	u64	nsecs;
} __attribute__((packed));

int main (int argc, char **argv)
{
	int trace;
	int options;
	char buf[4096];
	int rc;
	
	trace = open(tracing_file("trace_pipe"), O_RDONLY);
	if (trace == -1) {
		perror("trace_pipe");
		exit(2);
	}
	enable = open(tracing_file("events/syscalls/sys_enter/enable"), O_WRONLY);
	if (enable == -1) {
		perror("events/syscalls/sys_enter/enable");
		exit(2);
	}
	rc = write(enable, "1\n", 2);
	if (rc == -1) {
		perror("write enable");
		exit(2);
	}

#if 0
	options = open(tracing_file("trace_options"), O_WRONLY);
	if (options == -1) {
		perror("trace_options");
		exit(2);
	}
	rc = write(options, "bin\n", 4);
	if (rc == -1) {
		perror("write option for binary format");
		exit(2);
	}
#endif
/*
Try dumping in just hex or decimal - have to find PID.
Time is correct
PID is now known
*/
	for (;;) {
//			pwrite(toggle, "0", 1, 0);
HERE;
		rc = read(trace, buf, sizeof(buf));
PRd(rc);
//			pwrite(toggle, "0", 1, 0);
		if (rc == -1) exit(2);
		if (rc == 0) exit(0);
#if 0
		{
		sys_enter_format_s *f;
		
		for (f = (sys_enter_format_s *)buf;
		     f < (sys_enter_format_s *)&buf[rc]; f++) {
			printf("type %x flags %x count %x pid %d depth %d id %ld",
				f->type, f->flags, f->preempt_count,
				f->pid, f->lock_depth, f->id);
			printf(" args: %lx %lx %lx %lx %lx %lx",
				f->args[0], f->args[1], f->args[2],
				f->args[3], f->args[4], f->args[5]);
			printf("  %lld.%lld\n", f->nsecs / 1000000000, f->nsecs % 1000000000 / 1000); // need to add in 500 to get rounding right
		}
		}
#endif
#if 0
		{
		sys_enter_format_s *f;

		for (f = (sys_enter_format_s *)buf;
		     f < (sys_enter_format_s *)&buf[rc]; f++) {
			u16 *u = (u16 *)f;
			int i;
			for (i = 0; i < 20; i++) {
				printf(" %5d", u[i]);
			}
			printf("  %lld.%lld\n", f->nsecs / 1000000000, f->nsecs % 1000000000 / 1000); // need to add in 500 to get rounding right
		}
		}
#endif
#if 1
		{
		trace_s *t;
		for (t = (trace_s *)buf;
		     t < (trace_s *)&buf[rc]; t++) {
		     	printf("pid %5d %2d", t->pid, t->id);
			printf("  %lld.%lld\n", t->nsecs / 1000000000, t->nsecs % 1000000000 / 1000); // need to add in 500 to get rounding right
		}
		}
#endif
	}
	return 0;
}
