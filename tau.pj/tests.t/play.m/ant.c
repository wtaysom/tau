/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <eprintf.h>
#include <style.h>

enum {	MAX_LEVEL   = 4,
	MAX_RECS    = 4,
	SPLIT       = MAX_RECS / 2,
	NUM_BUCKETS = 17,
	MAX_MSG_Q   = 7 };

typedef struct Rec_s {
	u32	key;
	u32	val;
} Rec_s;

typedef struct Msg_s {
	u32	id;
	u32	op;
	Rec_s	r;
} Msg_s;

typedef struct Ant_s	Ant_s;
struct Ant_s {
	Ant_s	*next;
	u32	id;
	u16	level;
	u16	n;
	u32	first;
	Rec_s	r[MAX_RECS];
};
	
u32 Root;
Msg_s	Msg_q[MAX_MSG_Q];
Msg_s	*In_msg = Msg_q;
Msg_s	*Out_msg = Msg_q;
Msg_s	NilMsg = { 0 };

void send (Msg_s msg)
{
	Msg_s	*next = In_msg + 1;

	if (next == &Msg_q[MAX_MSG_Q]) next = Msg_q;
	if (next == Out_msg) fatal("full");
	*next = msg;
	In_msg = next;
}

Msg_s recv (void)
{
	Msg_s	msg;

	if (In_msg == Out_msg) return NilMsg;
	msg = *Out_msg;
	++Out_msg;
	if (Out_msg == &Msg_q[MAX_MSG_Q]) Out_msg = Msg_q;
	return msg;
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

Ant_s *new_ant (void)
{
	static u32	id = 0;
	Ant_s	*a = ezalloc(sizeof(*a));

	a->id = ++id;
	add_ant(a);
	return a;
}

void insert (u32 id, unint level, Rec_s r);

void split (Ant_s *a)
{
	Ant_s	*b = new_ant();
	Rec_s	r;
	unint	i;
	
	b->level = a->level;
	for (i = 0; i < a->n - SPLIT; i++) {
		b->r[i] = a->r[SPLIT + i];
	}
	b->n = i;
	a->n = SPLIT;
	r.key = b->r[0].key;
	r.val = b->id;
	insert(Root, b->level - 1, r);
}

void new_root (Ant_s *a)
{
	Ant_s	*root = new_ant();
	
	root->first = Root;
	Root = root->id;
}

void insert (u32 id, unint level, Rec_s r)
{
	Ant_s	*a;
	u32	i;

	if (id == 0) {
		a = new_ant();
		Root = a->id;
	} else {
		a = find_ant(id);
	}
	if (a->level < level) {	/* Need a new root */
		new_root(a);
		insert(Root, level, r);
		return;
	}	
	if (level < a->level) {
		for (i = 0; i < a->n; i++) {
			if (r.key < a->r[i].key) {
				insert(a->r[i].val, level, r);
				return;
			}
		}
	}
	if (a->n == MAX_RECS) {
		split(a);
		insert(Root, level, r);
		return;
	}
	for (i = 0; i < a->n; i++) {
		if (r.key < a->r[i].key) {
			memmove( &a->r[i+1], &a->r[i], a->n - i);
			break;
		}
	}
	// Check equality
	a->r[i] = r;
	++a->n;
}

static void pr_level (unint level)
{
	unint	i;

	for (i = level; i < MAX_LEVEL; i++) {
		printf("  ");
	}
}

void print (u32 id)
{
	Ant_s	*a = find_ant(id);
	unint	i;

	if (!a) return;
	pr_level(a->level);
	printf("id=%d level=%d n=%d\n", a->id, a->level, a->n);
	for (i = 0; i < a->n; i++) {
		pr_level(a->level);
		printf("%3ld. %d %d\n", i, a->r[i].key, a->r[i].val);
		if (a->level) {
			print(a->r[i].val);
		}
	}
}

void recv_loop (void)
{
	Msg_s	msg;
	Ant_s	*ant;

	for (;;) {
		msg = recv();
		if (!msg.id) return;
		a = find_ant(msg.id);
		if (!a) fatal("can't find ant %d", msg.id);
		a->type->op[msg.op](a, msg);
	}
}

int main (int argc, char *argv[])
{
	Rec_s	r;
	u32	i;

	for (i = 0; i < 4; i++) {
		r.key = genkey();
		r.val = genval();
		insert(Root, 0, r);
	}
	print(Root);
	return 0;
}
