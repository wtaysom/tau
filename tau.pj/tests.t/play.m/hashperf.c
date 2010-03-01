#include <stdio.h>

#include <style.h>
#include <crc.h>
#include <timer.h>

char *Name[] = {
	"this is one string",
	"another",
	"word",
	"more words",
	"a",
	NULL
};

enum {	MULTIPLIER = 31,
	NHASH      = 1543
};

unint hash (char *s)
{
	unint	h;
	u8	*p;

	h = 0;
	for (p = (u8 *)s; *p; p++) {
		h = MULTIPLIER * h + *p;
	}
	return h % NHASH;
}

int main (int argc, char *argv[])
{
	u64	start, finish;
	char	**name;
	unint	i;

	start = nsecs();
	name = Name;
	for (i = 0; i < 1000; i++) {
		if (!*name) {
			name = Name;
		}
		hash_string_32(*name);
	}
	finish = nsecs();
	printf("%lld\n", finish - start);
	return 0;
}
