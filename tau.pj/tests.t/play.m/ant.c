/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <debug.h>
#include <eprintf.h>
#include <style.h>

enum {	MAX_LEVEL   = 4,
	MAX_RECS    = 4,
	MAX_TWIGS   = MAX_RECS - 1,
	SPLIT_RECS  = MAX_RECS / 2,
	SPLIT_TWIGS = MAX_TWIGS / 2,
	NUM_BUCKETS = 17,
	MAX_MSG_Q   = 7 };

enum {	PRINT_OP = 0,
	INSERT_OP,
	NUM_OPS };

typedef struct Ant_s	Ant_s;
typedef struct Msg_s	Msg_s;

typedef void (*op_f)(Ant_s *a, Msg_s m);

typedef struct Rec_s {
	u32	key;
	u32	val;
} Rec_s;

typedef struct Twig_s {
	u32	key;
	u32	id;
} Twig_s;

struct Msg_s {
	u32	id;
	u16	op;
	union {
		struct {
			u16	level;
			u32	root;
			Rec_s	r;
		};
	};
};

typedef struct Type_s {
	u32	numops;
	op_f	op[];
} Type_s;

struct Ant_s {
	Ant_s	*next;
	Type_s	*type;
	u32	id;
	union {
		struct {
			u16	n;
			u16	level;
			u32	first;
			Twig_s	twig[MAX_TWIGS];
		};
		struct {
			u16	n;
			Rec_s	rec[MAX_RECS];
		};
		u32	root;
	};
};

void print_root   (Ant_s *a, Msg_s m);
void print_leaf   (Ant_s *a, Msg_s m);
void print_branch (Ant_s *a, Msg_s m);
void insert_root  (Ant_s *a, Msg_s m);
void insert_leaf  (Ant_s *a, Msg_s m);
void insert_branch(Ant_s *a, Msg_s m);

Type_s Root   = { NUM_OPS, { print_root,   insert_root   }};
Type_s Leaf   = { NUM_OPS, { print_leaf,   insert_leaf   }};
Type_s Branch = { NUM_OPS, { print_branch, insert_branch }};

Msg_s	Msg_q[MAX_MSG_Q];
Msg_s	*In_msg = Msg_q;
Msg_s	*Out_msg = Msg_q;
Msg_s	NilMsg = { 0 };

bool Msg_trace = TRUE;

static Ant_s	*Bucket[NUM_BUCKETS];

static Ant_s *hash (u32 id)
{
	return (Ant_s *)&Bucket[id % NUM_BUCKETS];
}

Ant_s *find_ant (u32 id)
{
	Ant_s	*a = hash(id)->next;

	while (a) {
		if (id == a->id) break;
		a = a->next;
	}
	return a;
}

void delete_ant (u32 id)
{
	Ant_s	*prev = hash(id);
	Ant_s	*a = prev->next;

	while (a) {
		if (id == a->id) {
			prev->next = a->next;
			free(a);
			return;
		}
		prev = a;
		a = a->next;
	}
	fatal("Not found %ld\n", id);
}

void add_ant (Ant_s *a)
{
	Ant_s	*prev = hash(a->id);

	a->next = prev->next;
	prev->next = a;
}

Ant_s *new_ant (Type_s *type)
{
	static u32	id = 0;
	Ant_s	*a = ezalloc(sizeof(*a));

	a->id = ++id;
	a->type = type;
	add_ant(a);
	return a;
}

void prmsg (char *label, Msg_s m)
{
	printf("%s %3d  op %2d  level %2d  root %3d  key %4d  val %4d\n",
		label, m.id, m.op, m.level, m.root, m.r.key, m.r.val);
}

void send (u32 id, Msg_s m)
{
	Msg_s	*next = In_msg + 1;

	m.id = id;
	if (next == &Msg_q[MAX_MSG_Q]) next = Msg_q;
	if (next == Out_msg) fatal("full");
	*In_msg = m;
	In_msg = next;
	if (Msg_trace) prmsg("Send", m);
}

Msg_s recv (void)
{
	Msg_s	m;

	if (In_msg == Out_msg) return NilMsg;
	m = *Out_msg;
	++Out_msg;
	if (Out_msg == &Msg_q[MAX_MSG_Q]) Out_msg = Msg_q;
	if (Msg_trace) prmsg("Recv", m);
	return m;
}

void recv_loop (void)
{
	Msg_s	m;
	Ant_s	*a;

	for (;;) {
		m = recv();
		if (!m.id) return;
		a = find_ant(m.id);
		if (!a) fatal("can't find ant %d", m.id);
		a->type->op[m.op](a, m);
	}
}

void split_leaf (Ant_s *a, Msg_s m)
{
FN;
	Ant_s	*b = new_ant( &Leaf);
	unint	i;

	b->level = a->level;
	for (i = 0; i < a->n - SPLIT_RECS; i++) {
		b->rec[i] = a->rec[SPLIT_RECS + i];
	}
	b->n = i;
	a->n = SPLIT_RECS;
	m.level = b->level - 1;
	m.r.key = b->rec[0].key;
	m.r.val = b->id;
	send(m.root, m);
}

void split_branch (Ant_s *a, Msg_s m)
{
FN;
	Ant_s	*b = new_ant( &Branch);
	unint	i;

	b->level = a->level;
	for (i = 0; i < a->n - SPLIT_TWIGS; i++) {
		b->rec[i] = a->rec[SPLIT_TWIGS + i];
	}
	b->n = i;
	a->n = SPLIT_TWIGS;
	m.level = b->level - 1;
	m.r.key = b->rec[0].key;
	m.r.val = b->id;
	send(m.root, m);
}

void insert_leaf (Ant_s *a, Msg_s m)
{
FN;
	u32	i;

	if (a->n == MAX_RECS) {
		split_leaf(a, m);
		send(m.root, m);
		return;
	}
	for (i = 0; i < a->n; i++) {
		if (m.r.key < a->rec[i].key) {
			memmove( &a->rec[i+1], &a->rec[i], a->n - i);
			break;
		}
	}
	// Check equality
	a->rec[i] = m.r;
	++a->n;
}

void insert_branch (Ant_s *a, Msg_s m)
{
FN;
	unint	i;

	for (i = 0; i < a->n; i++) {
		if (m.r.key < a->rec[i].key) {
			break;
		}
	}
	send(a->rec[i - 1].val, m);
}

void insert_root (Ant_s *a, Msg_s m)
{
FN;
	if (!a->root) {
		Ant_s *leaf = new_ant( &Leaf);
		a->root = leaf->id;
	}
	send(a->root, m);
}

void insert (u32 id, Rec_s r)
{
FN;
	Msg_s	m;

	m.op    = INSERT_OP;
	m.level = 0;
	m.root  = id;
	m.r     = r;
	send(id, m);
	recv_loop();
}

void print_tree(u32 id);

static void pr_level (unint level)
{
	unint	i;

	for (i = level; i < MAX_LEVEL; i++) {
		printf("  ");
	}
}

void print_leaf (Ant_s *a, Msg_s m)
{
FN;
	unint	i;

	pr_level(a->level);
	printf("LEAF id=%d level=%d n=%d\n", a->id, a->level, a->n);
	for (i = 0; i < a->n; i++) {
		pr_level(a->level);
		printf("%3ld. %d %d\n", i, a->rec[i].key, a->rec[i].val);
	}
}

void print_branch (Ant_s *a, Msg_s m)
{
FN;
	unint	i;

	pr_level(a->level);
	printf("id=%d level=%d n=%d\n", a->id, a->level, a->n);
	print_tree(a->first);
	for (i = 0; i < a->n; i++) {
		print_tree(a->rec[i].val);
	}
}

void print_root (Ant_s *a, Msg_s m)
{
FN;
	print_tree(a->root);
}

void print_tree (u32 id)
{
	Msg_s	m;

	zero(m);
	m.op    = PRINT_OP;
	m.level = 0;
	send(id, m);
	recv_loop();
}

u32 genkey (void)
{
	static u32	key = 0;
	return ++key;
}

u32 genval (void)
{
	static u32	val = 7;
	return ++val;
}

int main (int argc, char *argv[])
{
	Ant_s	*root;
	Rec_s	r;
	u32	i;

	root = new_ant( &Root);
	for (i = 0; i < 5; i++) {
		r.key = genkey();
		r.val = genval();
		insert(root->id, r);
	}
	print_tree(root->id);
	return 0;
}
