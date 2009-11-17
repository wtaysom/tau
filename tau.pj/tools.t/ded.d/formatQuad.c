#include <ncurses.h>
#include "ded.h"

/* Quads */

void up (void);
void down (void);

enum {
	u64S_PER_ROW = u8S_PER_ROW/sizeof(u64),
	COLUMN_WIDTH = sizeof(u64) * DIGITS_PER_u8 + 1
};

static void display (void)
{
	unint	i, j;
	unint	row;
	unint	col;
	unint	rowHigh;
	unint	colHigh;
	u64	*quads;

	rowHigh = ByteInBlock/u8S_PER_ROW;
	colHigh = (ByteInBlock%u8S_PER_ROW)/sizeof(u64);

	quads = (u64 *)Block;
	for (i = 0, row = START_ROW; i < NumRows; ++i, ++row) {
		mvprintw(row, 0, "%3x:", i * u8S_PER_ROW);
		for (j = 0, col = START_COL;
			j < u64S_PER_ROW;
			j++, col += COLUMN_WIDTH, quads++)
		{
			if ((i == rowHigh) && (j == colHigh)) {
				attron(A_REVERSE);
			}
			mvprintw(row, col, "%*qx", COLUMN_WIDTH, *quads);
			attroff(A_REVERSE);
		}
	}
}

static void copy (void)
{
	push(((u64 *)Block)[ByteInBlock/sizeof(u64)]);
}

static void left (void)
{
	unint	quadInBlock;

	quadInBlock = ByteInBlock / sizeof(u64);

	if (quadInBlock % u64S_PER_ROW != 0)
	{
		--quadInBlock;
	}
	ByteInBlock = quadInBlock * sizeof(u64);
}

static void right (void)
{
	unint	quadInBlock;

	quadInBlock = ByteInBlock / sizeof(u64);

	if (quadInBlock % u64S_PER_ROW != u64S_PER_ROW-1)
	{
		++quadInBlock;
	}
	ByteInBlock = quadInBlock * sizeof(u64);
}

static void next (void)
{
	unint	quadInBlock;

	if (ByteInBlock >= BlockSize - sizeof(u64)) return;

	quadInBlock = ByteInBlock / sizeof(u64) + 1;

	ByteInBlock = quadInBlock * sizeof(u64);
}

static void prev (void)
{
	unint	quadInBlock;

	if (ByteInBlock <= 0) return;

	quadInBlock = ByteInBlock / sizeof(u64) - 1;

	ByteInBlock = quadInBlock * sizeof(u64);
}

Format_s FormatQuad = {
	"Quads", display, copy, left, right, up, down, next, prev
};
