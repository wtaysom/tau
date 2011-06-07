#include <stdio.h>

void a(void);	// With this declaration, I don't have to put "void" in the
		// definition of the function.

void a() { printf("Hello\n"); }

int main (int argc, char *argv[]) {
	a();
	return 0;
}
