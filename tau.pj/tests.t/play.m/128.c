#include <stdio.h>

#include <style.h>

int main (int argc, char *argv[])
{
//#if 1 /* Not all targets support 128 bits */
#if __x86_64__ == 1
	__int128_t	x;
	__uint128_t	y = -1;

	printf("sizeof(int128)=%ld\n", sizeof(x));
	printf("uint128=%llx%llx\n", (u64)(y>>64), (u64)y);
#endif
	return 0;
}
