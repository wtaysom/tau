#include <ncurses.h>
#include "ded.h"

/* Bytes */

void up (void);
void down (void);

enum {
	COLUMN_WIDTH = sizeof(u8) * DIGITS_PER_u8 + 1,
	COL_PER_ROW  = 64
};

static void display (void)
{
	unint	i, j;
	unint	row;
	unint	col;
	unint	rowHigh;
	unint	colHigh;
	unint	numRows;
	unint	numCols;
	u8	*bytes;

	numCols = COL_PER_ROW;
	numRows = BlockSize / sizeof(u8) / numCols;

	rowHigh = ByteInBlock/COL_PER_ROW;
	colHigh = COL_PER_ROW;

	bytes = (u8 *)Block;
	for (i = 0, row = START_ROW; i < numRows; i++, row++) {
		row = START_ROW + i;
		mvprintw(row, 0, "%3x", i * numCols);
		for (j = 0, col = START_COL-1;
			j < numCols;
			j++, col+=COLUMN_WIDTH, bytes++)
		{
			if ((i == rowHigh) && (j == colHigh)) {
				attron(A_REVERSE);
			}
			mvprintw(row, col, "%*x", COLUMN_WIDTH, *bytes);
			attroff(A_REVERSE);
		}
	}
}

static void copy (void)
{
	push(((u8 *)Block)[ByteInBlock/sizeof(u8)]);
}

static void left (void)
{
	unint	byteInBlock;

	byteInBlock = ByteInBlock / sizeof(u8);

	if (byteInBlock % u8S_PER_ROW != 0)
	{
		--byteInBlock;
	}
	ByteInBlock = byteInBlock * sizeof(u8);
}

static void right (void)
{
	unint	byteInBlock;

	byteInBlock = ByteInBlock / sizeof(u8);

	if (byteInBlock % u8S_PER_ROW != u8S_PER_ROW-1)
	{
		++byteInBlock;
	}
	ByteInBlock = byteInBlock * sizeof(u8);
}

static void next (void)
{
	unint	byteInBlock;

	if (ByteInBlock >= BlockSize - sizeof(u8)) return;

	byteInBlock = ByteInBlock / sizeof(u8) + 1;

	ByteInBlock = byteInBlock * sizeof(u8);
}

static void prev (void)
{
	unint	byteInBlock;

	if (ByteInBlock <= 0) return;

	byteInBlock = ByteInBlock / sizeof(u8) - 1;

	ByteInBlock = byteInBlock * sizeof(u8);
}

Format_s FormatByte = {
	"Bytes", display, copy, left, right, up, down, next, prev
};
