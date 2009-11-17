#include <stdio.h>

#include <style.h>

#define X(_z) S_ ## _F ## _z
#define C(_x, _y)	{ enum { zSIZE = _y }; }
#define D(_w)		C(1, _w)

//C(__LINE__, 2);
//C(__LINE__, 4);
void xyzzy (void) {
D(2)
D(2)
}

#define G(x, l)	size ## x ## l
#define F(x, l) enum { G(x, l) = x }
#define E(x) F(x, __LINE__)
E(2);
E(3);

int main (int argc, char *argv[])
{
	printf("%d\n", size219);
	printf("%d\n", size320);
	return 0;
}
