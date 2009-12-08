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

#include "msg_debug.h"

#define PR_LEVEL	KERN_DEBUG	// KERN_ALERT

enum { NUM_TASKS = 26 };

typedef struct task_s {
	unint		tk_depth;
	void		*tk_task;
} task_s;


bool	Tau_debug_is_on     = 1;
bool	Tau_debug_func      = 0;
bool	Tau_debug_trace     = 0;
bool	Tau_debug_dieing    = 0;
bool	Tau_debug_filter    = 0;
bool	Tau_debug_show_only = 0;
bool	Tau_debug_delay     = 0;
u64	Tau_debug_mask      = tALL; //tFS | tKACHE;

EXPORT_SYMBOL(Tau_debug_is_on);
EXPORT_SYMBOL(Tau_debug_func);
EXPORT_SYMBOL(Tau_debug_trace);
EXPORT_SYMBOL(Tau_debug_filter);
EXPORT_SYMBOL(Tau_debug_show_only);
EXPORT_SYMBOL(Tau_debug_delay);
EXPORT_SYMBOL(Tau_debug_mask);

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
	"tau*",
	0
};

static bool Save_tau_debug_func;

static void lock (void)
{
	if (current == Current) {
		++Nesting;
		return;
	}
	spin_lock_irqsave( &Log_lock, Flags);
	Current = current;
	Nesting = 1;
	Save_tau_debug_func = Tau_debug_func;
	Tau_debug_func = FALSE;
}

static void unlock (void)
{
	--Nesting;
	if (!Nesting) {
		Tau_debug_func = Save_tau_debug_func;
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
	if (Tau_debug_delay) {
		prdelay(DelayMsec);
	}
}

static void display (void)
{
	printk(PR_LEVEL "%s", Line);
	delay();
}

static void log (void)
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

static int remainder (void)
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

	r = remainder();
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
	if (Tau_debug_filter) {
		if (Tau_debug_show_only) {
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

int tau_pr (const char *fn, unsigned line, const char *fmt, ...)
{
	va_list	args;
	task_s	*t;

	if (!Tau_debug_is_on) return 1;

	lock();

	t = get_task();
	indent(t->tk_depth);

	putlog('a' + (t - Task_pool));
	putlog(' ');

	prfn(fn, line);

	va_start(args, fmt);
	vformat(fmt, args);
	va_end(args);
	log();
	unlock();
	return 1;
}
EXPORT_SYMBOL(tau_pr);

int tau_label (const char *fn, unsigned line, const char *label)
{
	tau_pr(fn, line, " %s\n", label);
	return 1;
}
EXPORT_SYMBOL(tau_label);

int tau_here (const char *fn, unsigned line)
{
	tau_pr(fn, line, "\n");
	return 1;
}
EXPORT_SYMBOL(tau_here);

int tau_prfn (const char *fn, u64 fnmask)
{
	u64	mask;

	if (!Tau_debug_func) return 1;
	mask = Tau_debug_mask;
	if (mask != tALL) {
		if (fnmask == tALL) return 1;
		mask &= fnmask;
	}
	if (mask) tau_pr(fn, 0, "\n");
	return 1;
}
EXPORT_SYMBOL(tau_prfn);

int tau_prexit (const char *fn, unsigned line)
{
	if (Tau_debug_func) tau_pr(fn, line, "<<<\n");
	return 1;
}
EXPORT_SYMBOL(tau_prexit);

int tau_prd (const char *fn, unsigned line, const char *label, s64 x)
{
	tau_pr(fn, line, " %s=%lld\n", label, x);
	return 1;
}
EXPORT_SYMBOL(tau_prd);

int tau_prp (const char *fn, unsigned line, const char *label, const void *x)
{
	if (x) {
		tau_pr(fn, line, " %s=%p\n", label, x);
	} else {
		tau_pr(fn, line, " %s=NULL\n", label);
	}
	return 1;
}
EXPORT_SYMBOL(tau_prp);

int tau_prs (const char *fn, unsigned line, const char *label, const char *s)
{
	tau_pr(fn, line, " %s=%s\n", label, s);
	return 1;
}
EXPORT_SYMBOL(tau_prs);

int tau_pru (const char *fn, unsigned line, const char *label, u64 x)
{
	tau_pr(fn, line, " %s=%llu\n", label, x);
	return 1;
}
EXPORT_SYMBOL(tau_pru);

int tau_prx (const char *fn, unsigned line, const char *label, u64 x)
{
	tau_pr(fn, line, " %s=%llx\n", label, x);
	return 1;
}
EXPORT_SYMBOL(tau_prx);

int tau_prguid (const char *fn, unsigned line, const char *label, guid_t x)
{
	char	guid[38];

	sprintf(guid,
		"%.2x%.2x%.2x%.2x-%.2x%.2x-%.2x%.2x-"
		"%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x",
		x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7],
		x[8], x[9], x[10], x[11], x[012], x[13], x[14], x[15]);

	tau_pr(fn, line, " %s=%s\n", label, guid);
	return 1;
}
EXPORT_SYMBOL(tau_prguid);

int tau_prmem (const char *label, const void *addr, unint bytes)
{
	const u32	*p = addr;
	unsigned	i;

	if (!Tau_debug_is_on) return 1;

	label = munge_label(label);
	if (!label) return 1;

	lock();

	printk(PR_LEVEL "%s:\n", label);
	for (i = 15; i < bytes; i += 16, p += 4) {
		format("... %8p %8x %8x %8x %8x\n",
			p, p[0], p[1], p[2], p[3]);
		log();
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
	log();
	unlock();
	return 1;
}
EXPORT_SYMBOL(tau_prmem);

int tau_enter (const char *fn)
{
	task_s	*t;

	if (!Tau_debug_func) return 1;
	t = get_task();
	++t->tk_depth;
	tau_pr(fn, 0, "\n");
	return 1;
}
EXPORT_SYMBOL(tau_enter);

static void pr_ret (const char *fn, unsigned line, const char *fmt, ...)
{
	va_list	args;
	task_s	*t;

	if (!Tau_debug_func) return;

	lock();

	t = get_task();
	indent(t->tk_depth);

	putlog('a' + (t - Task_pool));
	putlog(' ');

	prfn(fn, line);

	va_start(args, fmt);
	vformat(fmt, args);
	va_end(args);
	log();
	if (t->tk_depth) {
		--t->tk_depth;
	} else {
		printk(PR_LEVEL "stack underflow at %s\n", fn);
	}
	unlock();
}

void tau_vret (const char *fn, unsigned line)
{
	pr_ret(fn, line, "\n");
}
EXPORT_SYMBOL(tau_vret);

snint tau_iret (const char *fn, unsigned line, snint x)
{
	pr_ret(fn, line, "%ld\n", x);
	return x;
}
EXPORT_SYMBOL(tau_iret);

u64 tau_qret (const char *fn, unsigned line, u64 x)
{
	pr_ret(fn, line, "%llx\n", x);
	return x;
}
EXPORT_SYMBOL(tau_qret);

void *tau_pret (const char *fn, unsigned line, void *x)
{
	pr_ret(fn, line, "%p\n", x);
	return x;
}
EXPORT_SYMBOL(tau_pret);

int tau_assert (const char *func, const char *what)
{
	printk(KERN_ALERT "ASSERT ERROR in %s @ %s\n", func, what);
	BUG();
	return 1;
}
EXPORT_SYMBOL(tau_assert);

int q_assert (const char *func, const char *what)
{
	printk(KERN_ALERT "QUEUE ASSERT ERROR in %s @ %s\n", func, what);
	BUG();
	return 1;
}
EXPORT_SYMBOL(q_assert);

#include <tau/msg.h>
#include "msg_internal.h"

char	*Requests[] = {
	"call",
	"create_gate",
	"destroy_gate",
	"destroy_key",
	"duplicate_key",
	"node_id",
	"plug_key",
	"receive",
	"send",
};

static char *gate_type (unsigned type)
{
	switch (type & (ONCE | PERM)) {
	case 0:			return "RESOURCE";
	case ONCE:
	case ONCE|PERM:		return "REPLY";
	case PERM:		return "REQUEST";
	}
	return "UNKNOWN KEY TYPE";
}

static char *gate_pass (unsigned type)
{
	switch (type & PASS_ANY) {
	case 0:			return "";
	case PASS_REPLY:	return "PASS_REPLY";
	case PASS_OTHER:	return "PASS_OTHER";
	case PASS_ANY:		return "PASS_ANY";
	}
	return "BAD PASS RESTRICTION";
}

static char *gate_data (unsigned type)
{
	switch (type & (READ_DATA | WRITE_DATA)) {
	case 0:				return "";
	case READ_DATA:			return "READ_DATA";
	case WRITE_DATA:		return "WRITE_DATA";
	case READ_DATA | WRITE_DATA:	return "RW_DATA";
	default:			return "I give up";
	}
}

static char *err_str (int rc)
{
	if (rc < 0) rc = -rc;

	switch (rc) {
	case 0:			return "OK";
	case DESTROYED:		return "Destruction notification";
	case EBADKEY:		return "Key is invalid";
	case ENOMSGS:		return "Out of message buffers";
	case EBADGATE:		return "Bad gate type";
	case ENODUP:		return "Can't duplicate key";
	case EBADID:		return "Bad gate id";
	case EBADINDEX:		return "Bad key index";
	case ECANTPASS:		return "Can't pass key on this key";
	case EREALLYBAD:	return "A really bad thing happened";
	case EBROKEN:		return "Gate is broken";
	case EDATA:		return "Error in data addresses";
	case ETOOBIG:		return "Trying to move too much data";
	default:
		printk(PR_LEVEL "unknown error %d\n", rc);
		return "Unknown Error";
	}
}

enum { BYTES_PER_LINE = 16 };

void print (const char *fmt, ...)
{
	va_list	args;

	lock();

	va_start(args, fmt);
	vformat(fmt, args);
	va_end(args);

	log();
	unlock();
}

static void pr_line (u8 *b)
{
	uint	i;
	int	c;

	format("    ");
	for (i = 0; i < BYTES_PER_LINE; i++) {
		format("%2x ", b[i]);
	}
	for (i = 0; i < BYTES_PER_LINE; i++) {
		c = b[i];
		if (!isascii(c) || !isprint(c)) c = '.';
		putlog(c);
	}
	putlog('\n');
	log();
}

int pr_body (void *body)
{
	u8	*b = body;
	int	i;

	lock();
	for (i = 0; i < BODY_SIZE/BYTES_PER_LINE; i++) {
		pr_line(b);
		b += BYTES_PER_LINE;
	}
	unlock();
	return 1;
}

int pr_key (char *s, void *key)
{
	key_s	*k = key;

	if (k->k_id) {
		print("%s key %s %s %s id=%llx "
			"node slot=%d\n",
			s, gate_type(k->k_type), gate_data(k->k_type),
			gate_pass(k->k_type), k->k_id, k->k_node);
	}
	return 1;
}

void tau_pr_packet (void *packet)
{
	packet_s	*p = packet;
	sys_s		*q = &p->pk_sys;

	print("    error=%ld flags=%lx length=%llx offset=%llx\n",
		p->pk_error, p->pk_flags, q->q_length, q->q_offset);
	pr_key("    dest  ", &p->pk_key);
	pr_key("    passed", &p->pk_passed_key);
	pr_body(p->pk_body);
}
EXPORT_SYMBOL(tau_pr_packet);	

void pr_mac (void *mac)
{
	u8	*a = mac;

	print("mac=%.2x%.2x%.2x%.2x%.2x%.2x\n",
		a[0], a[1], a[2], a[3], a[4], a[5]);
}

static void pr_datagate (datagate_s *gate)
{
	if (gate->gt_type & (READ_DATA | WRITE_DATA)) {
		format("    %s start=%p length=%lld\n",
			gate_data(gate->gt_type),
			gate->dg_start,
			gate->dg_length);
	}
}

void pr_gate (datagate_s *gate)
{
	if (!gate) return;
	lock();
	format("gate: %llx %s %s ->%s next=%p\n",
		gate->gt_id,
		gate_type(gate->gt_type),
		gate_pass(gate->gt_type), 
		gate->gt_avatar->av_name,
		gate->gt_siblings.next);
	pr_datagate((datagate_s *)gate);
	log();
	unlock();
}

static void pr_key_index (avatar_s *avatar, ki_t key_index)
{
	key_s		key;
	datagate_s	gate;
	int		rc;

	if (!key_index) return;
	rc = read_key(avatar, key_index, &key);
	if (rc) {
		format(" %u BAD KEY!!!\n", key_index);
		return;
	}
	if (key.k_node) {
		format(" %u->%llx %s %s "
			"slot=%d\n",
			key_index, key.k_id,
			gate_type(key.k_type),
			gate_pass(key.k_type), 
			key.k_node);
	} else {
		rc = read_gate(key.k_id, &gate);
		if (rc) {
			format(" %u->%llx %s %s Gate Broken\n",
				key_index, key.k_id,
				gate_type(key.k_type),
				gate_pass(key.k_type));
		} else {
			format(" %u->%llx %s %s ->%s\n",
				key_index, gate.gt_id,
				gate_type(gate.gt_type),
				gate_pass(gate.gt_type), 
				gate.gt_avatar->av_name);
			pr_datagate( &gate);
		}
	}
}	

static void pr_passed_key (avatar_s *avatar, unsigned key_index)
{
	if (!key_index) return;
	format("  passed key");
	pr_key_index(avatar, key_index);
}

static void in (avatar_s *avatar, const char *op)
{
	if (avatar) {
		format(" IN  %s<%d> %s",
			avatar->av_name, current->pid, op);
		avatar->av_last_op = op;
	} else {
		format("  IN %s avatar not set", op);
	}
}

static void out (avatar_s *avatar, int rc)
{
	if (avatar) {
		format(" OUT %s<%d> %s=%s",
			avatar->av_name, current->pid, avatar->av_last_op,
			err_str(rc));
	} else {
		format(" OUT <no avatar><%d> %s",
			current->pid, err_str(rc));
	}
	if (rc) {
		format("[%d]", rc);
	}
}

void in_call (
	avatar_s	*avatar,
	ki_t		key_index,
	msgbuf_s	*mb)
{
	if (!Tau_debug_trace) return;
	lock();

	in(avatar, "call");
	pr_key_index(avatar, key_index);
	format("  method=%u\n", mb->mb_body.m_method);
	pr_body( &mb->mb_body);

	log();
	unlock();		
}

void out_call (avatar_s *avatar, msgbuf_s *mb, int rc)
{
	if (!Tau_debug_trace) return;
	lock();

	out(avatar, rc);

	format(" tag=%p method=%u\n",
		mb->mb_packet.pk_sys.q_tag, mb->mb_body.m_method);

	if (!rc) {
		pr_passed_key(avatar, mb->mb_packet.pk_sys.q_passed_key);
		pr_body( &mb->mb_body);
	}
	log();
	unlock();	
}

void in_change_index (
	avatar_s	*avatar,
	ki_t		key_index,
	ki_t		std_index)
{
	if (!Tau_debug_trace) return;
	lock();

	in(avatar, "change_index");
	pr_key_index(avatar, key_index);
	format("  std index=%u\n", std_index);

	log();
	unlock();		
}

void out_change_index (avatar_s *avatar, int rc)
{
	if (!Tau_debug_trace) return;
	lock();

	out(avatar, rc);
	putlog('\n');

	log();
	unlock();	
}

void in_create_gate (avatar_s *avatar, msg_s *msg)
{
	if (!Tau_debug_trace) return;
	lock();

	in(avatar, "create_gate");
	if (msg->q.q_type & (READ_DATA | WRITE_DATA)) {
		format(" tag=%p %s %s %s\n"
			"\tstart=%p length=%llu\n",
			msg->q.q_tag,
			gate_type(msg->q.q_type), gate_data(msg->q.q_type),
			gate_pass(msg->q.q_type),
			msg->q.q_start, msg->q.q_length);
	} else {
		format(" tag=%p %s %s\n",
			msg->q.q_tag,
			gate_type(msg->q.q_type), gate_pass(msg->q.q_type));
	}
	log();
	unlock();
}

void out_create_gate (avatar_s *avatar, ki_t key_index, u64 gate_id, int rc)
{
	if (!Tau_debug_trace) return;
	lock();

	out(avatar, rc);
	format(" index=%d id=%llx\n", key_index, gate_id);
	log();
	unlock();
}

void in_destroy_gate (avatar_s *avatar, u64 id)
{
	if (!Tau_debug_trace) return;
	lock();

	in(avatar, "destroy_gate");
	format(" id=%llx\n", id);
	log();
	unlock();
}

void out_destroy_gate (avatar_s *avatar, int rc)
{
	if (!Tau_debug_trace) return;
	lock();

	out(avatar, rc);
	putlog('\n');
	log();
	unlock();
}

void in_destroy_key (avatar_s *avatar, ki_t key_index)
{
	if (!Tau_debug_trace) return;
	if (avatar->av_dieing && !get_key(avatar, key_index)) return;
	lock();

	in(avatar, "destroy_key");
	pr_key_index(avatar, key_index);
	log();
	unlock();
}

void out_destroy_key (avatar_s *avatar, int rc)
{
	if (!Tau_debug_trace) return;
	lock();

	out(avatar, rc);
	putlog('\n');
	log();
	unlock();
}

void in_duplicate_key (avatar_s *avatar, ki_t key_index)
{
	if (!Tau_debug_trace) return;
	lock();

	in(avatar, "duplicate_key");

	pr_key_index(avatar, key_index);
	log();
	unlock();
}

void out_duplicate_key (avatar_s *avatar, ki_t key_index, int rc)
{
	if (!Tau_debug_trace) return;
	lock();

	out(avatar, rc);
	putlog('\n');
	if (!rc) pr_passed_key(avatar, key_index);
	log();
	unlock();
}

void in_node_died (avatar_s *avatar, u64 node)
{
	if (!Tau_debug_trace) return;
	lock();

	in(avatar, "node_died ");
	format("%lld\n",  node);

	log();
	unlock();
}

void out_node_died (avatar_s *avatar)
{
	if (!Tau_debug_trace) return;
	lock();

	out(avatar, 0);
	putlog('\n');
	log();
	unlock();
}

static char *plug_str (unsigned plug)
{
	switch (plug) {
	case PLUG_CLIENT:	return "CLIENT";
	case PLUG_NETWORK:	return "NETWORK";
	default:		return "Don't know";
	}
}

void in_plug_key (avatar_s *avatar, ki_t plug, msgbuf_s *mb)
{
	if (!Tau_debug_trace) return;
	lock();

	in(avatar, "plug_key ");
	format("%s",  plug_str(plug));

	pr_key_index(avatar, mb->mb_packet.pk_sys.q_passed_key);
	pr_body( &mb->mb_body);	
	log();
	unlock();
}

void out_plug_key (avatar_s *avatar, int rc)
{
	if (!Tau_debug_trace) return;
	lock();

	out(avatar, rc);
	putlog('\n');
	log();
	unlock();
}
	
void in_receive (avatar_s *avatar)
{
	if (!Tau_debug_trace) return;
	lock();

	in(avatar, "receive");
	putlog('\n');
	log();
	unlock();
}

void out_receive (avatar_s *avatar, msgbuf_s *mb, int rc)
{
	sys_s	*sys = &mb->mb_packet.pk_sys;

	if (!Tau_debug_trace) return;
	lock();

	out(avatar, rc);

	format(" tag=%p method=%u\n", sys->q_tag, mb->mb_body.m_method);

	if (!rc) {
		pr_passed_key(avatar, sys->q_passed_key);
		pr_body( &mb->mb_body);
	}	
	log();
	unlock();
}

void in_read_data (avatar_s *avatar, ki_t key_index, void *data)
{
	data_s	*d = data;

	if (!Tau_debug_trace) return;
	lock();

	in(avatar, "read_data");

	pr_key_index(avatar, key_index);
	format("    start=%p length=%llu offset=%llu\n",
		d->q_start, d->q_length, d->q_offset);
	log();
	unlock();
}

void out_read_data (avatar_s *avatar, int rc)
{
	if (!Tau_debug_trace) return;
	lock();

	out(avatar, rc);
	putlog('\n');
	log();
	unlock();
}

void in_send (avatar_s *avatar, ki_t key_index, msgbuf_s *mb)
{
	unint	passed_key = mb->mb_packet.pk_sys.q_passed_key;

	if (!Tau_debug_trace) return;
	lock();

	in(avatar, passed_key ? "send_key" : "send");

	pr_key_index(avatar, key_index);
	format("  method=%d\n", mb->mb_body.m_method);
	pr_passed_key(avatar, passed_key);
	pr_body( &mb->mb_body);		
	log();
	unlock();
}

void out_send (avatar_s *avatar, int rc)
{
	if (!Tau_debug_trace) return;
	lock();

	out(avatar, rc);
	putlog('\n');
	log();
	unlock();
}

void in_stat_key  (avatar_s *avatar, ki_t key_index)
{
	if (!Tau_debug_trace) return;
	lock();

	in(avatar, "stat_key");

	pr_key_index(avatar, key_index);
	log();
	unlock();
}

void out_stat_key (avatar_s *avatar, msg_s *msg, int rc)
{
	if (!Tau_debug_trace) return;
	lock();

	out(avatar, rc);
	format(	"\n\tgate id = %llx"
		"\n\tnode no = %llx"
		"\n\tlength  = %llx"
		"\n\ttype    = %llx\n",
		msg->sk_gate_id,
		msg->sk_node_no,
		msg->sk_length,
		msg->sk_type);
	putlog('\n');
	log();
	unlock();
}

void in_write_data (avatar_s *avatar, ki_t key_index, void *data)
{
	data_s	*d = data;

	if (!Tau_debug_trace) return;
	lock();

	in(avatar, "write_data");

	pr_key_index(avatar, key_index);
	format("    start=%p length=%llu offset=%llu\n",
		d->q_start, d->q_length, d->q_offset);
	log();
	unlock();
}

void out_write_data (avatar_s *avatar, int rc)
{
	if (!Tau_debug_trace) return;
	lock();

	out(avatar, rc);
	putlog('\n');
	log();
	unlock();
}

