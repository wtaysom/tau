#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <ncurses.h>
#include <getopt.h>

#include <eprintf.h>

#include "ded.h"

bool	Initialized = FALSE;

void finish (int sig)
{
	endwin();
	exit(0);
}

void setSignals (void)
{
	signal(SIGHUP,	finish);
	signal(SIGINT,	SIG_IGN);
	signal(SIGQUIT,	finish);
	signal(SIGTRAP,	finish);
	signal(SIGABRT,	finish);
	signal(SIGFPE,	finish);
	signal(SIGILL,	finish);
	signal(SIGBUS,	finish);
	signal(SIGSEGV,	finish);
	signal(SIGTSTP,	finish);
}

void usage (char *msg)
{
	fprintf(stderr,
		"error %s\n"
		"usage: %s [-dtv] [-b <block size>] <device>\n"
		"\td - debug\n"
		"\tv - zero fill (not yet implemented)\n"
		"\t? - usage\n"
		"\tb - change the default (4096) block size\n"
		"\t\tb 128\n"
		"\t\tb 256\n"
		"\t\tb 512\n"
		"\t\tb 1024\n"
		"\t\tb 2048\n"
		"\t\tb 4096\n"
		"\tWhen using %s, type \"?\" for commands.\n",
		msg, getprogname(), getprogname());
	exit(2);
}

int sysError (char *s)
{
	int	rc = errno;

	sprintf(Error, "%s: %s", s, strerror(errno));
	if (!Initialized) {
		usage(Error);
	}
	return rc;
}

int myError (char *s)
{
	sprintf(Error, "%s", s);
	if (!Initialized) {
		fprintf(stderr, "ERROR: %s\n", Error);
	}
	return -1;
}

bool	Debug = FALSE;
bool	Zerofill = FALSE;
int	NumRows;

int init (int argc, char *argv[])
{
	if (BlockSize > MAX_BLOCK_SIZE) {
		return myError("Block size too big");
	}
	NumRows = BlockSize / u8S_PER_ROW;

	setSignals();

	initscr();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	//idlok(stdscr, TRUE);
	keypad(stdscr, TRUE);

	initStack();
	initMemory();

	Initialized = TRUE;
	return 0;
}

int main (int argc, char *argv[])
{
	int	c;
	int	rc;

	setprogname(argv[0]);

	while ((c = getopt(argc, argv, "dvb:")) != -1) {
		switch (c) {
		case 'b':
			BlockSize = atoi(optarg);
			switch (BlockSize) {
			case 128:
			case 256:
			case 512:
			case 1024:
			case 2048:
			case 4096:
				break;
			default:
				usage("bad block size");
				break;
			}
			break;
		case 'd':
			Debug = TRUE;
			break;
		case 'v':
			Zerofill = TRUE;
			break;
		case '?':
		case ':':
		default:
			usage("bad option");
			break;
		}
	}

	rc = initDisk(argv[optind]);
	if (rc) {
		usage("couldn't open device");
	}

	if (init(argc, argv) == -1) {
		finish(0);
		usage("initialization failed");
	}
	input();

	finish(0);

	return 0;
}
