/****************************************************************************
 |  (C) Copyright 2008 Novell, Inc. All Rights Reserved.
 |
 |  GPLv2: This program is free software; you can redistribute it
 |  and/or modify it under the terms of version 2 of the GNU General
 |  Public License as published by the Free Software Foundation.
 |
 |  This program is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 +-------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <style.h>
#include <debug.h>
#include <kmem.h>

#include <mdb.h>


typedef void	(*cmdfunc_t)(int argc, char *arg[]);

typedef struct Cmd_s {
	char		*c_name;
	cmdfunc_t	 c_func;
	char		*c_help;
} Cmd_s;

typedef struct Arg_s {
	void	(*a_cmd)(void);
	addr	a_addr;
	snint	a_offset;
	unint	a_lines;
	u64	a_pattern;
} Arg_s;

extern Cmd_s	Cmd[];

static void do_nothing (void)
{}

Arg_s	Arg = { do_nothing, 0, 0, DEFAULT_LINES, 0 };
Cmd_s	*Current;

static void error (void)
{
	printf("%s\n", Current->c_help);
}

static void set_lines (char *arg)
{
	unint	lines;

	if (arg) {
		lines = eval(arg);
		if (lines) {
			Arg.a_lines = lines;
		}
	}
	if (Arg.a_lines > MAX_LINES) Arg.a_lines = MAX_LINES;
}

Cmd_s *cmdLookup (char *name)
{
	Cmd_s	*cmd;

	for (cmd = Cmd; cmd->c_name; ++cmd) {
		if (strcmp(name, cmd->c_name) == 0) {
			return cmd;
		}
	}
	return NULL;
}

void invokeCmd (int argc, char *argv[])
{
	Cmd_s	*cmd;

	if (argc == 0) {
		if (Arg.a_cmd) Arg.a_cmd();
		return;
	}
	cmd = cmdLookup(argv[0]);
	if (cmd == 0) {
		printf("%s not found\n", argv[0]);
		return;
	}
	Current = cmd;
	cmd->c_func(argc, argv);
}

static void dump (int argc, char *argv[])
{
	dumpsyms();
}

	/*
	 * help: print list of commands
	 */
static void help (int argc, char *argv[])
{
	Cmd_s	*cmd;

	printf("<address> can be hex number like 'abc' or a variable name\n");
	for (cmd = Cmd; cmd->c_name; ++cmd) {
		printf("%s\n", cmd->c_help);
	}
}

static bool c_args (int argc, char *argv[], addr *address, u64 *value)
{
	if (argc != 3) {
		fprintf(stderr, "requires 2 args: %s <address> <value>", argv[0]);
		return FALSE;
	}
	*address = eval(argv[1]);
	if (*address == 0) {
		fprintf(stderr, "%s is not a valid address\n", argv[1]);
		return FALSE;
	}
	*value = eval(argv[2]);
	return TRUE;
}

static void cb (int argc, char *argv[])
{
	u64	x;
	addr	address;

	if (c_args(argc, argv, &address, &x)) {
		w8(address, (u8)x);
	}
}

static void cd (int argc, char *argv[])
{
	u64	x;
	addr	address;

	if (c_args(argc, argv, &address, &x)) {
		w32(address, (u32)x);
	}
}

static void cq (int argc, char *argv[])
{
	u64	x;
	addr	address;

	if (c_args(argc, argv, &address, &x)) {
		w64(address, (u64)x);
	}
}

static bool d_args (int argc, char *argv[])
{
	addr	address = Arg.a_addr;
	char	*arg;

	arg = *++argv;
	if (arg) {
		address = eval(arg);
		if (!address) {
			fprintf(stderr, "invalid expression: %s\n", arg);
			return FALSE;
		}
		arg = *++argv;
	}
	set_lines(arg);
	Arg.a_addr  = address;
	return TRUE;
}

static unint db_line (addr address)
{
	printf("%lx:", address);
	line8(address);
	printf("  |  ");
	linechar(address);
	printf("\n");

	return U8_LINE;
}

static void db_repeat (void)
{
	unint	i;
	addr	address = Arg.a_addr;

	for (i = 0; i < Arg.a_lines; ++i) {
		address += db_line(address);
	}
	Arg.a_addr = address;
}

static void db (int argc, char *argv[])
{
	if (d_args(argc, argv)) {
		Arg.a_cmd = db_repeat;
		db_repeat();
	}
}

static unint dd_line (addr address)
{
	printf("%lx:", address);
	line32(address);
	printf("  |  ");
	linechar(address);
	printf("\n");

	return U8_LINE;
}

static void dd_repeat (void)
{
	unint	i;
	addr	address = Arg.a_addr;

	for (i = 0; i < Arg.a_lines; ++i) {
		address += dd_line(address);
	}
	Arg.a_addr = address;
}

static void dd (int argc, char *argv[])
{
	if (d_args(argc, argv)) {
		Arg.a_cmd = dd_repeat;
		dd_repeat();
	}
}

static unint dq_line (addr address)
{
	printf("%lx:", address);
	line64(address);
	printf("  |  ");
	linechar(address);
	printf("\n");

	return U8_LINE;
}

static void dq_repeat (void)
{
	unint	i;
	addr	address = Arg.a_addr;

	for (i = 0; i < Arg.a_lines; ++i) {
		address += dq_line(address);
	}
	Arg.a_addr = address;
}

static void dq (int argc, char *argv[])
{
	if (d_args(argc, argv)) {
		Arg.a_cmd = dq_repeat;
		dq_repeat();
	}
}

static void ds_repeat (void)
{
	unint	i;
	addr	address = Arg.a_addr;

	for (i = 0; i < Arg.a_lines; ++i) {
		address += prstack(address);
	}
	Arg.a_addr = address;
}

static void ds (int argc, char *argv[])
{
	if (d_args(argc, argv)) {
		Arg.a_cmd = ds_repeat;
		ds_repeat();
	}
}

static bool l_args (int argc, char *argv[])
{
	addr	address = Arg.a_addr;
	int	offset  = Arg.a_offset;
	char	*arg;

	arg = *++argv;
	if (arg) {
		if (arg[0] == '+') {	// offset
			offset = eval( &arg[1]);
			arg = *++argv;
		} else {
			offset = 0;
		}
	}
	if (arg) {
		address = eval(arg);
		if (!address) {
			fprintf(stderr, "invalid expression: %s\n", arg);
			return FALSE;
		}
		arg = *++argv;
	}
	set_lines(arg);
	Arg.a_addr   = address;
	Arg.a_offset = offset;
	return TRUE;
}

static void l_repeat (void)
{
	addr	address = Arg.a_addr;
	unint	i;

	for (i = 0; i < Arg.a_lines; ++i) {
		printf("%lx:", address);
		address += lineaddr(address);
		printf("\n");
	}
	Arg.a_addr = raddr(Arg.a_addr + Arg.a_offset);
}

static void l (int argc, char *argv[])
{
	if (l_args(argc, argv)) {
		Arg.a_cmd = l_repeat;
		l_repeat();
	}
}


static bool s_args (int argc, char *argv[])
{
	addr	address = Arg.a_addr;
	u64	pattern = Arg.a_pattern;
	char	*arg;

	arg = *++argv;
	if (arg) {
		pattern = eval(arg);
		arg = *++argv;
	}
	if (arg) {
		address = eval(arg);
		if (!address) {
			fprintf(stderr, "invalid expression: %s\n", arg);
			return FALSE;
		}
		arg = *++argv;
	}
	set_lines(arg);
	Arg.a_addr    = address;
	Arg.a_pattern = pattern;
	return TRUE;
}

static addr sd_search (addr address)
{
	u32	x;
	u32	pattern = Arg.a_pattern;

	for (;;) {
		x = r32(address);
		if (x == pattern) {
			printf("%lx: %.8x\n", address, pattern);
			return address + sizeof(u32);
		}
		address += sizeof(u32);
	}
}

static void sd_repeat (void)
{
	unint	i;
	addr	address = Arg.a_addr;

	for (i = 0; i < Arg.a_lines; ++i) {
		address = sd_search(address);
	}
	Arg.a_addr = address;
}

static void sd (int argc, char *argv[])
{
	if (s_args(argc, argv)) {
		Arg.a_cmd = sd_repeat;
		sd_repeat();
	}
}

#if 0
static void ns (int argc, char *argv[])
{
	u64	addr;

	if (argc != 2) {
		error();
		return;
	}
	addr = findsym(argv[1]);
	printf("%s=%llx\n", argv[1], addr);
}
#endif

static void find (int argc, char *argv[])
{
	if (argc != 2) {
		error();
		return;
	}
	start_search();
	for (;;) {
		Sym_s	*sym;
		sym = search(argv[1]);
		if (!sym) return;
		printf("%llx %s\n", sym_address(sym), sym_name(sym));
	}
}

static void pr_sym (Sym_s *sym)
{
	if (sym) {
		printf("%8llx: %s\n", sym_address(sym), sym_name(sym));
	}
}

static void where_repeat (void)
{
	addr	address = Arg.a_addr;
	u32	offset;
	Sym_s	*sym;

	sym = findname(address);
	if (sym) {
		pr_sym(prevaddress(sym));
		offset = address - sym_address(sym);
		if (offset) {
			printf("%8lx: %s+0x%x\n", address, sym_name(sym), offset);
		} else {
			printf("%8lx: %s\n", address, sym_name(sym));
		}
		pr_sym(nextaddress(sym));
	} else {
		printf("%8lx: ?\n", address);
	}
}

static void where (int argc, char *argv[])
{
	if (d_args(argc, argv)) {
		Arg.a_cmd = where_repeat;
		where_repeat();
	}
}

static void pid (int argc, char *argv[])
{
	unsigned	pid;
	addr		task;

	if (argc != 2) {
		error();
		return;
	}
	pid = strtoul(argv[1], NULL, 0);
	task = pid2task(pid);
	if (task) {
		printf("%lx\n", task);
	} else {
		printf("pid %d not found\n", pid);
	}
}

static void q (int argc, char *argv[])
{
	exit(0);
}

#if DISASM IS_ENABLED
static bool u_args (int argc, char *argv[])
{
	addr	address = Arg.a_addr;
	char	*arg;

	arg = *++argv;
	if (arg) {
		address = eval(arg);
		if (!address) {
			fprintf(stderr, "invalid expression: %s\n", arg);
			return FALSE;
		}
		arg = *++argv;
	}
	set_lines(arg);
	Arg.a_addr  = address;
	return TRUE;
}

static void pr_label (addr address)
{
	Sym_s	*sym;

	sym = findname(address);

	if ((sym) && (sym_address(sym) == address)) {
		printf("%s:\n", sym_name(sym));
	}
}

static void u_repeat (void)
{
	unint	i;
	addr	address = Arg.a_addr;

	for (i = 0; i < Arg.a_lines; ++i) {
		pr_label(address);
		address += disasm(address);
	}
	Arg.a_addr = address;
}

static void u (int argc, char *argv[])
{
	if (u_args(argc, argv)) {
		Arg.a_cmd = u_repeat;
		u_repeat();
	}
}
#endif

Cmd_s	Cmd[] =
{
{ "q",    q,   "quit                       : exit program"},
{ "help", help,"help                       : print this message"},
{ "dump", dump,"dump                       : dump symbol table"},
{ "cb",   cb,	"cb <address> <value>       : change a byte"},
{ "cd",   cd,	"cd <address> <value>       : change 32 bits"},
{ "cq",   cq,	"cq <address> <value>       : change 64 bits"},
{ "db",   db,	"db <address> [<num lines>] : display bytes"},
{ "dd",   dd,	"dd <address> [<num lines>] : display 32 bits"},
{ "dq",   dq,	"dq <address> [<num lines>] : display 64 bits"},
{ "ds",   ds,	"ds <address> [<num lines>] : dump stack"},
//{ "ns", ns,	"ns <name>"},
{ "?",    where,"?  <address>               : find symbols near address" },
{ "l",    l,	"l [+<offset>] <address> [<num lines>] : follow linked list"},
{ "sd",   sd,	"sd <pattern> <from> [<num>] : search for 32 bit pattern"},
{ "find", find, "find <wild card exp>       : uses '*' and '?'" },
{ "pid",  pid,  "pid <pid>                  : finds task address for pid"},

#if DISASM IS_ENABLED
{ "u",    u,    "u <address> [<num lines>]  : disassemble"},
#endif

{ NULL, 0, NULL }
};
