#include <stdlib.h>
#include "ded.h"

void add ()
{
	push(pop() + pop());
}

void sub ()
{
	u64	n = pop();

	push(pop() - n);
}

void mul ()
{
	push(pop() * pop());
}

void divide ()
{
	u64	n = pop();

	if (n == 0)	push(0);
	else push(pop() / n);
}

void mod ()
{
	u64	n = pop();

	if (n == 0)	push(0);
	else push(pop() % n);
}

void neg ()
{
	push( -pop());
}

void not ()
{
	push( ~pop());
}

void and ()
{
	push(pop() & pop());
}

void or ()
{
	push(pop() | pop());
}

void xor ()
{
	push(pop() ^ pop());
}

void leftShift ()
{
	push(pop() << 1);
}

void rightShift ()
{
	push(pop() >> 1);
}

void qrand()
{
	u64	x;

	x  = ((u64)random()) << 48;
	x += ((u64)random()) << 32;
	x += ((u64)random()) << 16;
	x += random();
	push(x);
}
