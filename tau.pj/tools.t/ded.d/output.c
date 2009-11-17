#include <ctype.h>
#include <ncurses.h>
#include "ded.h"

char Error[MAX_ROW];
char BlankLine[] =	"          "
			"          "
			"          "
			"          "
			"          "
			"          "
			"          "
			"          ";

u64	Number;
u64	Lastx;
bool	NewNumber = FALSE;
Radix_t	Radix = HEX;

static char	*Help[] = {
	" ? help",
	" _ change sign",
	" + add",
	" - subtract",
	" * multiply",
	" / divide",
	" % modulo",
	" ~ not",
	" & and",
	" | or",
	" ^ xor",
	" < left shift",
	" > right shift",
	" . swap",
	" ! store",
	" = retrive",
	" # random",
	" h left",
	" j down",
	" k up",
	" l right",
	" g goto block",
	" q quit",
	" r change radix",
	" Z previous field",
	"^A next format",
	"^B backward a block",
	"^D exit",
	"^F forward a block",
	"^H clear digit",
	"^I (tab) next field",
	"^L refresh",
	"^M ENTER",
	"^P push current field",
	"^R Rotate stack",
	"^T zero stack",
	"^U last x",
	"^W previous format",
	"^X clr entry",
	"\n\nstrike any key to get back",
	0
};


void help (void)
{
	int		x, y;
	char	**s;

	clear();

	x = HELP_COL;
	y = HELP_ROW;
	for (s = Help; *s != NULL; ++s)	{
		mvaddstr(y, x, *s);
		x += HELP_SIZE;
		if (x > 80 - HELP_SIZE) {
			x = HELP_COL;
			++y;
		}
	}
	refresh();
	getch();
	clear();
}

void prError (void)
{
	if (Error[0]) {
		mvprintw(ERR_ROW, ERR_COL, "ERROR: %s", Error);
		Error[0] = '\0';
	}
}

void prStack (void)
{
attron(A_REVERSE);

	attroff(A_REVERSE);
	if (Radix == HEX) {
		attron(A_REVERSE);
		if (NewNumber) attron(A_UNDERLINE);
	}
	mvprintw(0, STK_COL, "%16qx", ith(0));
	attroff(A_REVERSE);
	attroff(A_UNDERLINE);

	if (Radix == UNSIGNED) {
		attron(A_REVERSE);
		if (NewNumber) attron(A_UNDERLINE);
	}
	mvprintw(0, STK_COL+16, " %20qu", ith(0));
	attroff(A_REVERSE);
	attroff(A_UNDERLINE);

	mvprintw(1, STK_COL, "%16qx %20qu", ith(1), ith(1));
}

void output (void)
{
	mvprintw(0, 0, BlankLine);
	mvprintw(1, 0, BlankLine);
	mvprintw(0, 0, "Block: %qx", BlockNum);
	mvprintw(1, 0, "%s", Current->title);
	prStack();
	prError();

	Current->display();

	move(0, 0);

	refresh();
}
