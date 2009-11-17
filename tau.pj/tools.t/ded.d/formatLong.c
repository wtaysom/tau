#include <ncurses.h>
#include "ded.h"

/* Longs */

void up (void);
void down (void);

enum {
	u32S_PER_ROW = u8S_PER_ROW/sizeof(u32),
	COLUMN_WIDTH = sizeof(u32) * DIGITS_PER_u8 + 1
};

static void display (void)
{
	unint	i, j;
	unint	row;
	unint	col;
	unint	rowHigh;
	unint	colHigh;
	u32	*longs;

	rowHigh = ByteInBlock/u8S_PER_ROW;
	colHigh = (ByteInBlock%u8S_PER_ROW)/sizeof(u32);

	longs = (u32 *)Block;
	for (i = 0, row = START_ROW; i < NumRows; i++, row++) {
		row = START_ROW + i;
		mvprintw(row, 0, "%3x:", i * u8S_PER_ROW);
		for (j = 0, col = START_COL;
			j < u32S_PER_ROW;
			j++, col+=COLUMN_WIDTH, longs++)
		{
			if ((i == rowHigh) && (j == colHigh)) {
				attron(A_REVERSE);
			}
			mvprintw(row, col, "%*lx", COLUMN_WIDTH, *longs);
			attroff(A_REVERSE);
		}
	}
}

static void copy (void)
{
	push(((u32 *)Block)[ByteInBlock/sizeof(u32)]);
}

static void left (void)
{
	unint	longInBlock;

	longInBlock = ByteInBlock / sizeof(u32);

	if (longInBlock % u32S_PER_ROW != 0)
	{
		--longInBlock;
	}
	ByteInBlock = longInBlock * sizeof(u32);
}

static void right (void)
{
	unint	longInBlock;

	longInBlock = ByteInBlock / sizeof(u32);

	if (longInBlock % u32S_PER_ROW != u32S_PER_ROW-1)
	{
		++longInBlock;
	}
	ByteInBlock = longInBlock * sizeof(u32);
}

static void next (void)
{
	unint	longInBlock;

	if (ByteInBlock >= BlockSize - sizeof(u32)) return;

	longInBlock = ByteInBlock / sizeof(u32) + 1;

	ByteInBlock = longInBlock * sizeof(u32);
}

static void prev (void)
{
	unint	longInBlock;

	if (ByteInBlock <= 0) return;

	longInBlock = ByteInBlock / sizeof(u32) - 1;

	ByteInBlock = longInBlock * sizeof(u32);
}

Format_s FormatLong = {
	"Longs", display, copy, left, right, up, down, next, prev
};
