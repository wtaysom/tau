#include <stdio.h>
#include <style.h>

enum { MAX_BLK_NUM = 1ULL << (sizeof(u64)*8 - 1),
	MAX_FILE_BLK = MAX_BLK_NUM - 1};

int main (int argc, char *argv[])
{
	printf("%llx\n", (u64)MAX_BLK_NUM);
	printf("%llx\n", (u64)MAX_FILE_BLK);
	return 0;
}
