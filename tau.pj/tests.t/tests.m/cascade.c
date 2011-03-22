#include <style.h>
#include <timer.h>

int main (int argc, char *argvp[])
{
	cascade_s	wheel;
	u64  delta;
	u64  start;
	u64  i;

//      printf("%lld\n", nsecs());
//      printf("%lld\n", nsecs());

	start = nsecs();
	for (i = 0; i < 400017LL; i++) {
		delta = nsecs() - start;
		start = nsecs();
		cascade(wheel, delta);
	}
	pr_cascade("test", wheel);
	return 0;
}
