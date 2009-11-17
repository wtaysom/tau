#include <sys/ioctl.h>

#include <ctype.h>
#include <ncurses.h>
#include "ded.h"

#define CNTRL(_x_)	((_x_) & 0x1f)

static unint	RadixTable[] = { 16, 10, 10, 256 };

void digit (int c, char offset)
{
	u64	x;

	if (NewNumber)	{
		x = pop();
		x *= RadixTable[Radix];
		x += c - offset;
	} else {
		NewNumber = TRUE;
		x = c - offset;
	}
	push(x);
}

void lastx (void)
{
	Lastx = peek();
	NewNumber = FALSE;
}

unint	Height;
unint	Width;

int get_term_size (void)
{
	struct winsize windowSize;

	if (ioctl(0, TIOCGWINSZ, &windowSize) < 0) {
		return -1;
	}
	Height = windowSize.ws_row;
	Width = windowSize.ws_col;
	return 0;
}

void input (void)
{
	int	c;
	u64	x;
	u64	new_block = 0;

	NewNumber = TRUE;
	Radix = HEX;

	output();

	for (;;)	{

		if (new_block != BlockNum) {
			if (readDisk(new_block) != 0) {
				new_block = BlockNum;
			}
		}
		output();

		c = getch();

		switch (c) {
			/*
			 * Don't use these: CNTRL(C), CNTRL(Z), CNTRL(S),
			 *		CNTLR(Q), CNTRL(Y)
			 */
		case '?':			help();		break;
		case '_':	lastx();	neg();		break;
		case '+':	lastx();	add();		break;
		case '-':	lastx();	sub();		break;
		case '*':	lastx();	mul();		break;
		case '/':	lastx();	divide();	break;
		case '%':	lastx();	mod();		break;
		case '~':	lastx();	not();		break;
		case '&':	lastx();	and();		break;
		case '|':	lastx();	or();		break;
		case '^':	lastx();	xor();		break;
		case '<':	lastx();	leftShift();	break;
		case '>':	lastx();	rightShift();	break;
		case '.':	lastx();	swap();		break;
		case '!':	lastx();	store();	break;
		case '=':	lastx();	retrieve();	break;
		case '#':	lastx();	qrand();	break;

		case KEY_RESIZE:	/* Screen resize event - don't do anything */
				break;	

		case KEY_LEFT:
		case 'h':	Current->left(); /* left one column */
				break;

		case KEY_DOWN:
		case 'j':	Current->down(); /* down one row */
				break;

		case KEY_UP:
		case 'k':	Current->up(); /* up one row */
				break;

		case KEY_RIGHT:						
		case 'l':	Current->right(); /* right one column */
				break;

		case CNTRL('A'):clear(); /* switch to next format */
				next_format();
				break;

		case CNTRL('W'):clear(); /* switch to previous format */
				prev_format();
				break;

		case CNTRL('B'):/* Backward one block */
				if (BlockNum > 0) {
					new_block = BlockNum - 1;
				}
				break;

		case EOF:	/* Ignore EOF - send on resize */
				break;
		case 'q':
		case CNTRL('D'):finish(0); /* Exit */
				break;

		case CNTRL('F'):/* Forward a block */
				new_block = BlockNum + 1;
				break;

		case 'g':	/* Go to specified Block */
				lastx();
				x = pop();
				NewNumber = FALSE;
				new_block = x;
				break;

		case KEY_BACKSPACE:
		case KEY_DC:
		case CNTRL('H'):/* Delete last digit entered */
				push(pop() / RadixTable[Radix]);
				break;

		case CNTRL('I'):/* Next field */
				Current->next();
				break;

		case KEY_CLEAR:
		case CNTRL('L'):/* Refresh the display */
				wrefresh(curscr);
				break;

		case KEY_ENTER:
		case CNTRL('M'):/* Finish entry of number or duplicate top */
				if (!NewNumber)	{
					duptop();
				}
				NewNumber = FALSE;
				break;

		case CNTRL('P'):/* Push copy of current field */
				Current->copy();
				NewNumber = FALSE;
				break;

		case CNTRL('R'):/* Rotate from bottom of stack to top */
				rotate();
				break;

		case 'r':	/* Change radix */
				++Radix;
				if (Radix > UNSIGNED)	Radix = HEX;
				break;

		case CNTRL('T'):/* Reinitialize stack */
				initStack();
				NewNumber = TRUE;
				break;

		case CNTRL('U'):/* Push last x */
				push(Lastx);
				NewNumber = FALSE;
				break;

		case CNTRL('X'):/* Delete top of stack */
				(void)pop();
				push(0);
				NewNumber = TRUE;

		case 'Z':	/* Previous field */
				Current->prev();
				break;

		default:	/* Is it a digit? */
				if ((Radix == HEX) && isxdigit(c)) {
					if (isdigit(c)) {
						digit(c, '0');
					} else if (isupper(c)) {
						digit(c, 'A' - 10);
					} else {
						digit(c, 'a' - 10);
					}
				} else if ((Radix == UNSIGNED) && isdigit(c)) {
					digit(c, '0');
				} else if ((Radix == SIGNED) && isdigit(c)) {
					digit(c, '0');
				} else if (Radix == CHARACTER)	{
					digit(c, 0);
				} else {
					flash();
					sprintf(Error, "bad char=%d", c);
				}
				break;
		}
	}
}
