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

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/ctype.h>

#include "pdebug.h"

#define PR_LEVEL	KERN_DEBUG	// KERN_ALERT

enum { NUM_TASKS = 26 };

typedef struct task_s {
	unint		tk_depth;
	void		*tk_task;
} task_s;


bool	Pi_debug_is_on     = 1;
bool	Pi_debug_func      = 1;
bool	Pi_debug_filter    = 0;
bool	Pi_debug_show_only = 0;
bool	Pi_debug_delay     = 0;


static int	DelayMsec = 100;

static task_s	Task_pool[NUM_TASKS];
static task_s	*Next_task = Task_pool;

static DEFINE_SPINLOCK(Log_lock);
static unint	Flags;
static void	*Current;
static unint	Nesting;

static char	Line[1<<12];
static char	*Next_char = Line;
static char	*End = &Line[sizeof(Line)];
static char	Log[1<<16];
static char	*Next_log = Log;

static char *SkipList[] = {
	0
};

static char *ShowOnlyList[] = {
	"pi*",
	0
};

static bool Save_pi_debug_func;

static void lock (void)
{
	if (current == Current) {
		++Nesting;
		return;
	}
	spin_lock_irqsave( &Log_lock, Flags);
	Current = current;
	Nesting = 1;
	Save_pi_debug_func = Pi_debug_func;
	Pi_debug_func = 0;
}

static void unlock (void)
{
	--Nesting;
	if (!Nesting) {
		Pi_debug_func = Save_pi_debug_func;
		Current = 0;
		spin_unlock_irqrestore( &Log_lock, Flags);
	}
}

static void prdelay (int msec)
{
	set_current_state(TASK_UNINTERRUPTIBLE);
	schedule_timeout(1+ ((msec * HZ)/1000));
}

static void delay (void)
{
	if (Pi_debug_delay) {
		prdelay(DelayMsec);
	}
}

static void display (void)
{
	printk(PR_LEVEL "%s", Line);
	delay();
}

static void commit_log (void)
{
	const char	*b = Line;

	if (Next_char == Line) return;

	*Next_char = '\0';
	display();

	for (;;) {
		if (Next_log == &Log[sizeof(Log)]) {
			Next_log = Log;
		}
		*Next_log++ = *b++;
		if (!*b) break;
	}
	Next_char = Line;
}

static int remaining (void)
{
	return End - Next_char;
}

static void putlog (char c)
{
	if (Next_char < End) {
		*Next_char++ = c;
	}
}

static void vformat (const char *fmt, va_list args)
{
	int	x;
	int	r;

	r = remaining();
	if (r <= 0) return;

	x = vsnprintf(Next_char, r, fmt, args);
	if (x > 0) Next_char += x;
}

static void format (const char *fmt, ...)
{
	va_list	args;

	va_start(args, fmt);
	vformat(fmt, args);
	va_end(args);
}

static int match (const char *p, const char *s)
{
	for (;;) {	
		switch (*p) {
		case '\0':	return *s == '\0';
		
		case '*':	for (;;) {
					++p;
					if (*p == '\0') return 1;
					if (*p != '*') break;
				}
				for (;;) {
					if (match(p, s)) return 1;
					++s;
					if (*s == '\0') return 0;
				}
					
		case '\\':	++p;
				if (*p == '\0') return 0;
				if (*p != *s) return 0;
				break;
				
		case '?':	if (*s == '\0') return 0;
				++s;
				break;
	
		default:	if (*p != *s) return 0;
				++s;
				break;
		}
		++p;
	}
}

static int skip_list (const char *v)
{
	char	**s;

	for (s = SkipList; *s; ++s) {
		if (match(*s, v)) {
			return 1;
		}
	}
	return 0;
}

static int show_only_list (const char *v)
{
	char	**s;

	for (s = ShowOnlyList; *s; ++s) {
		if (match(*s, v)) {
			return 1;
		}
	}
	return 0;
}


const char *file_name (const char *s)
{
	const char	*lastSlash = s;

	for (;;) {
		switch (*s++) {
		case '\0':	return lastSlash;

		case '/':	if (*s != '\0') lastSlash = s;
				break;

		case '\\':	if (*s != '\0') ++s;
				break;

		default:	break;	/* do nothing */
		}
	}
	return lastSlash;
}

static const char *munge_label (const char *label)
{
	if (label == NULL) return NULL;
	//label = file_name(label);
	if (Pi_debug_filter) {
		if (Pi_debug_show_only) {
			if (!show_only_list(label)) return NULL;
		}
		else if (skip_list(label)) return NULL;
	}
	return label;
}

static task_s *get_task (void)
{
	void	*task = current;
	task_s	*f;

	f = Next_task;
	for (;;) {
		if (f == Task_pool) {
			f = &Task_pool[NUM_TASKS];
		}
		--f;
		if (f->tk_task == task) return f;
		if (f == Next_task) {
			++Next_task;
			if (Next_task == &Task_pool[NUM_TASKS]) {
				Next_task = Task_pool;
			}
			if (f->tk_depth) {
				printk(PR_LEVEL "Wrapped tasks %p\n",
						f->tk_task);
			}
			f->tk_depth = 0;
			f->tk_task  = task;
			return f;
		}
	}
}

static void indent (unint depth)
{
	while (depth--) {
		putlog(' ');
		putlog(' ');
		putlog(' ');
		putlog(' ');
	}
}

static void prfn (const char *fn, unsigned line)
{
	if (line) {
		format("%s<%u>", fn, line);
	} else {
		format("%s", fn);
	}
}

int pi_pr (const char *fn, unsigned line, const char *fmt, ...)
{
	va_list	args;
	task_s	*t;

	if (!Pi_debug_is_on) return 1;

	lock();

	t = get_task();
	indent(t->tk_depth);

	putlog('a' + (t - Task_pool));
	putlog(' ');

	prfn(fn, line);

	va_start(args, fmt);
	vformat(fmt, args);
	va_end(args);
	commit_log();
	unlock();
	return 1;
}

int pi_label (const char *fn, unsigned line, const char *label)
{
	pi_pr(fn, line, " %s\n", label);
	return 1;
}

int pi_here (const char *fn, unsigned line)
{
	pi_pr(fn, line, "\n");
	return 1;
}

int pi_prfn (const char *fn)
{
	if (Pi_debug_func) pi_pr(fn, 0, "\n");
	return 1;
}

int pi_prexit (const char *fn, unsigned line)
{
	if (Pi_debug_func) pi_pr(fn, line, "<<<\n");
	return 1;
}

int pi_prd (const char *fn, unsigned line, const char *label, s64 x)
{
	pi_pr(fn, line, " %s=%lld\n", label, x);
	return 1;
}

int pi_prp (const char *fn, unsigned line, const char *label, const void *x)
{
	if (x) {
		pi_pr(fn, line, " %s=%p\n", label, x);
	} else {
		pi_pr(fn, line, " %s=NULL\n", label);
	}
	return 1;
}

int pi_prs (const char *fn, unsigned line, const char *label, const char *s)
{
	pi_pr(fn, line, " %s=%s\n", label, s);
	return 1;
}

int pi_pru (const char *fn, unsigned line, const char *label, u64 x)
{
	pi_pr(fn, line, " %s=%llu\n", label, x);
	return 1;
}

int pi_prx (const char *fn, unsigned line, const char *label, u64 x)
{
	pi_pr(fn, line, " %s=%llx\n", label, x);
	return 1;
}

int pi_prguid (const char *fn, unsigned line, const char *label, guid_t x)
{
	char	guid[38];

	sprintf(guid,
		"%.2x%.2x%.2x%.2x-%.2x%.2x-%.2x%.2x-"
		"%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x",
		x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7],
		x[8], x[9], x[10], x[11], x[012], x[13], x[14], x[15]);

	pi_pr(fn, line, " %s=%s\n", label, guid);
	return 1;
}

int pi_prmem (const char *label, const void *addr, unint bytes)
{
	const u32	*p = addr;
	unsigned	i;

	if (!Pi_debug_is_on) return 1;

	label = munge_label(label);
	if (!label) return 1;

	lock();

	printk(PR_LEVEL "%s:\n", label);
	for (i = 15; i < bytes; i += 16, p += 4) {
		format("... %8p %8x %8x %8x %8x\n",
			p, p[0], p[1], p[2], p[3]);
		commit_log();
	}
	switch (bytes % 16)
	{
	case 0:	break;
	case 1:	case 2: case 3: case 4:
		format("... %8p %8x\n", p, p[0]);
		break;
	case 5: case 6: case 7: case 8:
		format("... %8p %8x %8x\n", p, p[0], p[1]);
		break;
	case 9: case 10: case 11: case 12:
		format("... %8p %8x %8x %8x\n", p, p[0], p[1], p[2]);
		break;
	case 13: case 14: case 15:
		format("... %8p %8x %8x %8x %8\n", p, p[0], p[1], p[2], p[3]);
		break;
	}
	commit_log();
	unlock();
	return 1;
}

int pi_enter (const char *fn)
{
	task_s	*t;

	if (!Pi_debug_func) return 1;
	t = get_task();
	++t->tk_depth;
	pi_pr(fn, 0, "\n");
	return 1;
}

static void pr_ret (const char *fn, unsigned line, const char *fmt, ...)
{
	va_list	args;
	task_s	*t;

	if (!Pi_debug_func) return;

	lock();

	t = get_task();
	indent(t->tk_depth);

	putlog('a' + (t - Task_pool));
	putlog(' ');

	prfn(fn, line);

	va_start(args, fmt);
	vformat(fmt, args);
	va_end(args);
	commit_log();
	if (t->tk_depth) {
		--t->tk_depth;
	} else {
		printk(PR_LEVEL "stack underflow at %s\n", fn);
	}
	unlock();
}

void pi_vret (const char *fn, unsigned line)
{
	pr_ret(fn, line, "\n");
}

snint pi_iret (const char *fn, unsigned line, snint x)
{
	pr_ret(fn, line, "%ld\n", x);
	return x;
}

u64 pi_qret (const char *fn, unsigned line, u64 x)
{
	pr_ret(fn, line, "%llx\n", x);
	return x;
}

void *pi_pret (const char *fn, unsigned line, void *x)
{
	pr_ret(fn, line, "%p\n", x);
	return x;
}

int pi_assert (const char *func, const char *what)
{
	printk(KERN_ALERT "ASSERT ERROR in %s @ %s\n", func, what);
	BUG();
	return 1;
}

int q_assert (const char *func, const char *what)
{
	printk(KERN_ALERT "QUEUE ASSERT ERROR in %s @ %s\n", func, what);
	BUG();
	return 1;
}
