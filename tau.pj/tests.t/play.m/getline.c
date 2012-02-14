/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */
#include <stdio.h>
#include <stdlib.h>

#include <ncurses.h>

#include <eprintf.h>
#include <style.h>

#define CNTRL(_x_)	((_x_) & 0x1f)

enum { MAX_LINE = 1024 };

static void finish (void)
{
	endwin();
	exit(0);
}

void init (void)
{
	set_cleanup(finish);
	initscr();
	cbreak();
	noecho();
	nonl();
	idlok(stdscr, TRUE);
	keypad(stdscr, TRUE);
}

char *get_line (void)
{
	static char	line[MAX_LINE];
	static bool	is_initialized = FALSE;
	char	*end = &line[MAX_LINE-1];
	char	*next;
	int	c;

	if (!is_initialized) {
		init();
		is_initialized = TRUE;
	}
	for (next = line;;) {
		c = getch();
		printf("%d\n", c);
		switch (c) {
		case CNTRL('D'):
		case EOF:
			if (next == line) {
				return NULL;
			}
			*next = '\0';
			return line;
		default:
			if (next == end) continue;
			*next++ = c;
			break;
		}
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	int ch;

	initscr();			/* Start curses mode 		*/
	raw();				/* Line buffering disabled	*/
	keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
	noecho();			/* Don't echo() while we do getch */

    	printw("Type any character to see it in bold\n");
	ch = getch();			/* If raw() hadn't been called
					 * we have to press enter before it
					 * gets to the program 		*/
	if(ch == KEY_F(1))		/* Without keypad enabled this will */
		printw("F1 Key pressed");/*  not get to us either	*/
					/* Without noecho() some ugly escape
					 * charachters might have been printed
					 * on screen			*/
	else
	{	printw("The pressed key is ");
		attron(A_BOLD);
		printw("%c", ch);
		attroff(A_BOLD);
	}
	refresh();			/* Print it on to the real screen */
    	getch();			/* Wait for user input */
	endwin();			/* End curses mode		  */

	return 0;
}
#if 0
int main (int argc, char *argv[])
{
	char	*line;

	for (;;) {
		line = get_line();
		if (!line) break;
		printf("%s", line);
	}
	finish();
	return 0;
}
#endif
