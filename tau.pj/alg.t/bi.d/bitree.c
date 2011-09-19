/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <eprintf.h>
#include <style.h>

#include "bitree.h"

static BiNode_s *bi_new (Rec_s r) {
  BiNode_s *node = ezalloc(sizeof(*node));
  node->rec = r;
  return node;
}

int bi_audit(BiTree_s t) {
  return 0;
}

static void pr_indent (int indent) {
  while (indent-- > 0) {
    printf("    ");
  }
}

static void pr_lump (Lump_s a) {
  enum { MAX_PR_LUMP = 32 };
  u8 *p;
  int size = a.size;
  if (size > MAX_PR_LUMP) size = MAX_PR_LUMP;
  for (p = a.d; size != 0; --size, p++) {
    if (isprint(*p)) {
      putchar(*p);
    } else {
      putchar('.');
    }
  }
}

static void pr_rec (Rec_s r, int indent) {
  pr_indent(indent);
  pr_lump(r.key);
  printf(": ");
  pr_lump(r.val);
  printf("\n");
}

static void pr_node (BiNode_s *parent, int indent) {
  if (!parent) return;
  pr_node(parent->left, indent + 1);
  pr_rec(parent->rec, indent);
  pr_node(parent->right, indent + 1);
}

int bi_print (BiTree_s t) {
  pr_node(t.root, 0);
  return 0;
}

int bi_stats (BiTree_s t) {
  return 0;
}

int bi_find (BiTree_s t, Lump_s key, Lump_s *val) {
  return 0;
}

/* Rotations:
        y    right->   x
       / \   <-left   / \
      x   c          a   y
     / \                / \
    a   b              b   c
*/
static void rot_right (BiNode_s **np) {
  BiNode_s *x;
  BiNode_s *y;
  BiNode_s *tmp;
  y = *np;
  if (!y) return;
  x = y->left;
  if (!x) return;
  tmp = x->right;
  x->right = y;
  y->left = tmp;
  *np = x;
}
  
static void rot_left (BiNode_s **np) {
  BiNode_s *x;
  BiNode_s *y;
  BiNode_s *tmp;
  x = *np;
  if (!x) return;
  y = x->right;
  if (!y) return;
  tmp = y->left;
  y->left = x;
  x->right = tmp;
  *np = y;
}


static void insert (BiNode_s **np, Rec_s r) {
  BiNode_s *parent = *np;
  if (*np) {
    if (cmplump(parent->rec.key, r.key) < 0) {
      insert(&parent->left, r);
    } else {
      insert(&parent->right, r);
    }
  } else {
    *np = bi_new(r);
  }
}

int bi_insert (BiTree_s *t, Rec_s r) {
  insert(&t->root, r);
  return 0;
}

void delete (BiNode_s **np, Lump_s key) {
  BiNode_s *parent = *np;
  if (!parent) fatal("Key not found");
  int r = cmplump(parent->rec.key, key);
  if (r < 0) delete(&parent->left, key);
  if (r > 0) delete(&parent->right, key);
  if (!parent->right) {
    *np = parent->left;
    free(parent);
    return;
  }
  if (!parent->left) {
    *np = parent->right;
    free(parent);
    return;
  }
  rot_right(np);
  
}

int bi_delete (BiTree_s *t, Lump_s key) {
  delete(&t->root, key);
  return 0;
}

