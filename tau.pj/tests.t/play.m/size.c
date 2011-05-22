#include <stddef.h>
#include <stdio.h>

#include <style.h>


/* Never use the following two macros directly */
#define zLABEL_CHECK(_label, _line) SIZE_ ## _label ## _ ## _line
#define zCHECK_SIZE(_label, _line, _struct, _x)	\
	enum {zLABEL_CHECK(_label, _line) = 1 / ((_struct) == (_x)) }

#ifdef __x86_64__
#define zCHECK_STRUCT(_struct, _x32, _x64)	\
	zCHECK_SIZE(_struct, __LINE__, sizeof(_struct), _x64)

#define zCHECK_OFFSET(_struct, _field, _x32, _x64)	\
	zCHECK_SIZE(_struct, __LINE__, offsetof(_struct, _field), _x64)

#define zCHECK_DEFINE(_x, _x32, _x64)	\
	zCHECK_SIZE(_x, __LINE__, _x, _x64)

#endif

#ifdef __i386__
#define zCHECK_STRUCT(_struct, _x32, _x64)	\
	zCHECK_SIZE(_struct, __LINE__, sizeof(_struct), _x32)

#define zCHECK_OFFSET(_struct, _field, _x32, _x64)	\
	zCHECK_SIZE(_struct, __LINE__, offsetof(_struct, _field), _x32)

#define zCHECK_DEFINE(_x, _x32, _x64)	\
	zCHECK_SIZE(_x, __LINE__, _x, _x32)

#endif

typedef struct abc_s {
	long	a, b, c;
} abc_s;

#define Y	17

	zCHECK_STRUCT(abc_s, 12, 24);
	zCHECK_STRUCT(abc_s, 12, 24);
	zCHECK_OFFSET(abc_s, b, 4, 8);
	zCHECK_DEFINE(Y, 17, 17);

int main (int argc, char *argv[])
{
#ifdef __x86_64__
	printf("%d\n", SIZE_abc_s_42);
	printf("offsetof=%ld\n", offsetof(abc_s, b));
#endif
#ifdef __i386__
	printf("%d\n", SIZE_abc_s_42);
	printf("offsetof=%zd\n", offsetof(abc_s, b));
#endif
	return 0;
}
