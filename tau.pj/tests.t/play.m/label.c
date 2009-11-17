#include <stdio.h>

#include <style.h>

int main (int argc, char *argv[])
{
	void	*state = &&start;

	goto *state;
start:		printf("start\n"); state = &&state_2; goto *state;
state_1:	printf("state 1\n"); state = &&end; goto *state;
state_2:	printf("state 2\n"); state = &&state_1; goto *state;
end:		printf("end\n");
	return 0;
}
