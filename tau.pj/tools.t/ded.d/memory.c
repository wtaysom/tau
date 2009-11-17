#include <strings.h>
#include "ded.h"

u64	Memory[MAX_MEM];

void initMemory ()
{
	bzero(Memory, sizeof(Memory));
}

void store ()
{
	unint	i;

	i = pop();
	if (i >= MAX_MEM) return;

	duptop();

	Memory[i] = pop();
}

void retrieve ()
{
	unint	i;

	i = pop();
	if (i >= MAX_MEM) return;

	push(Memory[i]);
}
