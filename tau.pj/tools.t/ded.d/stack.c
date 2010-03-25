#include <string.h>
#include "ded.h"

static u64	Stack[STK_SIZE];

void initStack ()
{
	bzero(Stack, sizeof(Stack));
}

u64 peek ()
{
	return Stack[0];
}

u64 pop ()
{
	unint	i;
	u64	x = Stack[0];

	for (i = 0; i < STK_SIZE-1; ++i) {
		Stack[i] = Stack[i+1];
	}
	return x;
}

void push (u64 x)
{
	unint	i;

	for (i = STK_SIZE-1; i > 0; --i) {
		Stack[i] = Stack[i-1];
	}
	Stack[0] = x;
}

void swap ()
{
	u64	x = Stack[0];

	Stack[0] = Stack[1];
	Stack[1] = x;
}

void rotate ()
{
	unint	i;
	u64	x = Stack[0];

	for (i = 0; i < STK_SIZE-1; ++i) {
		Stack[i] = Stack[i+1];
	}
	Stack[STK_SIZE-1] = x;
}

void duptop ()
{
	push(Stack[0]);
}

u64 ith (unint i)
{
	if (i >= STK_SIZE) return 0;
	return Stack[i];
}
