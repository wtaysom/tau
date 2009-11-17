#include <ctype.h>
#include <ncurses.h>
#include "ded.h"

/* Chars */

void up(void);
void down(void);
void left(void);
void right(void);
void next(void);
void prev(void);

static void display (void)
{
	unint	i, j;
	unint	row;
	unint	rowHigh;
	unint	colHigh;
	char	*chars;

	rowHigh = ByteInBlock / u8S_PER_ROW;
	colHigh = ByteInBlock % u8S_PER_ROW;
	chars = (char *)Block;
	for (i = 0, row = START_ROW; i < NumRows; ++i, ++row) {
		mvprintw(row, 0, "%3x:", i * u8S_PER_ROW);
		for (j = 0; j < u8S_PER_ROW; j++, chars++) {
			if ((i == rowHigh) && (j == colHigh)) {
				attron(A_REVERSE);
			}
			mvprintw(row, START_COL+j, "%c", isgraph(*chars) ? *chars : '.');
			attroff(A_REVERSE);
		}
	}
}

static void copy (void)
{
	push(Block[ByteInBlock]);
}

Format_s FormatChar = {
	"Bytes", display, copy, left, right, up, down, next, prev
};
