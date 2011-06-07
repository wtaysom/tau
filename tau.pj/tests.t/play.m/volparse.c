#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <uuid/uuid.h>

#include <style.h>
#include <eprintf.h>

/*
 * Function to parse volume arguments from -o mount option.
 * <option> ::= <arg>[,<arg>]
 * <arg>    ::= <name>[=<value>]
 * <value>  ::= <number>|<string>|<uuid>
 *
 * Have to pass in a list of functions for processing specific
 * name.
 */

enum { MAX_STRING = 40 };

typedef int	(*opt_f)(char *value, void *arg);

typedef struct option_s {
	char	*opt_name;
	opt_f	opt_func;
	void	*opt_arg;
	bool	opt_set;
} option_s;

int string_opt (char *value, void *arg)
{
	int	n;

	n = strlen(value);
	if (n >= MAX_STRING) return -1;

	strncpy(arg, value, n);
	return 0;
}

int number_opt (char *value, void *arg)
{
	u64	n;

	n = strtoll(value, NULL, 0);
	*(u64 *)arg = n;
	return 0;
}

int guid_opt (char *value, void *arg)
{
	return uuid_parse(value, arg);
}

static char *get_token (char **cmdline, char *separator)
{
	char	*p = *cmdline;
	char	*token;
	int	c;

	while (isspace(*p)) {
		++p;
	}
	for (token = p;; p++) {
		c = *p;
		if (isspace(c)) {
			*p = '\0';
			do {
				c = *++p;
			} while (isspace(c));
			break;
		}
		if (c == '\0') break;
		if (c == '=') break;
		if (c == ',') break;
	}
	*p = '\0';
	*cmdline = ++p;
	*separator = c;
	return token;
}

bool isset_opt (option_s *options, char *name)
{
	option_s	*opt;

	for (opt = options; opt->opt_name; opt++) {
		if (strcmp(name, opt->opt_name) == 0) {
			return opt->opt_set;
		}
	}
	return -FALSE;
}

int set_opt (option_s *options, char *name, char *value)
{
	option_s	*opt;
	int		rc = 0;

	for (opt = options; opt->opt_name; opt++) {
		if (strcmp(name, opt->opt_name) == 0) {
			if (opt->opt_func) {
				rc = opt->opt_func(value, opt->opt_arg);
				return rc;
			}
			opt->opt_set = TRUE;
			return rc;
		}
	}
	return -1;
}

void parse_opt (option_s *options, char *cmdline)
{
	char	separator;
	char	*name;
	char	*value;
	int	rc;

	for (;;) {
		name = get_token( &cmdline, &separator);
		if (separator == '=') {
			value = get_token( &cmdline, &separator);
		} else {
			value = NULL;
		}
		rc = set_opt(options, name, value);
		if (rc) {
			warn("name=%s value=%s", name, value);
		}
		if (separator == '\0') break;
	}
}

void dump (option_s *options)
{
	option_s	*opt;

	for (opt = options; opt->opt_name; opt++) {
		printf("%s=%llx\n", opt->opt_name, *(u64 *)opt->opt_arg);
	}
}

int main (int argc, char *argv[])
{
	char	out[40];
	char	volname[100];

	u64	num;
	guid_t	guid;

	option_s	options[] = {
		{ "a", number_opt, &num, FALSE },
		{ "u", guid_opt, guid, FALSE },
		{ "vol", string_opt, volname, FALSE },
		{ 0, 0, 0, 0 }};

	printf("%s\n", argv[1]);
	parse_opt(options, argv[1]);
	dump(options);
	uuid_unparse(guid, out);
	printf("vol=%s %lld %s\n", volname, num, out);
	return 0;
}
