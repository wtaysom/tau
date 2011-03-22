
#include <style.h>

#if 0
#define MAGIC_STRING(_x_)       # _x_
#define MAKE_STRING(_x_)        MAGIC_STRING(_x_)

#define CONCATENATE_DETAIL(x, y) x##y
#define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)
#define MAKE_UNIQUE(x) CONCATENATE(x, __COUNTER__)
#define CHECK_CONST(_expression)	\
	enum { MAKE_UNIQUE(CHECK_) = 1 / (_expression) }
#endif

enum {	MAX_PID = 40000,
	SYSCALL_SHIFT = 8 };

CHECK_CONST((1 << SYSCALL_SHIFT) >= 200);
//CHECK_CONST((1 << SYSCALL_SHIFT) >= 325);

int main(int argc, char *argv[])
{
	return 0;
}
