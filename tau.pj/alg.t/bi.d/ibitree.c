/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <eprintf.h>
#include <style.h>

#include "ibitree.h"

struct iBiNode_s {
  iBiNode_s *left;
  iBiNode_s *right;
  u64 key;
};

int ibi_audit (iBiTree_s *tree) {
  return 0;
}

static void pr_indent (int indent) {
  while (indent-- > 0) {
    printf("    ");
  }
}

static void pr_key (u64 key, int indent) {
  pr_indent(indent);
  printf("%lld\n", key);
}

static void pr_node (iBiNode_s *node, int indent) {
  if (!node) return;
  pr_node(node->left, indent + 1);
  pr_key(node->key, indent);
  pr_node(node->right, indent + 1);
}

int ibi_print (iBiTree_s *tree) {
  pr_node(tree->root, 0);
  return 0;
}

void ibi_pr_path (iBiTree_s *tree, u64 key) {
  iBiNode_s *node = tree->root;
  while (node) {
    printf("%lld", node->key);
    if (key == node->key) {
      printf("\n");
      return;
    }
    printf(" ");
    if (key < node->key) {
      node = node->left;
    } else {
      node = node->right;
    }
  }
}

static void node_stat (iBiNode_s *node, iBiStat_s *s, int depth) {
  if (!node) return;
  ++s->num_nodes;
  if (depth > s->max_depth) {
    s->max_depth = depth;
    s->deepest = node->key;
  }
  s->total_depth += depth;
  if (node->left) {
    ++s->num_left;
    node_stat(node->left, s, depth + 1);
  }
  if (node->right) {
    ++s->num_right;
    node_stat(node->right, s, depth + 1);
  }
}
  
iBiStat_s ibi_stats (iBiTree_s *tree) {
  iBiStat_s stat = { 0 };
  node_stat(tree->root, &stat, 1);
  return stat;
}

int ibi_find (iBiTree_s *tree, u64 key) {
  iBiNode_s *node = tree->root;
  while (node) {
    if (key == node->key) {
      return 0;
    }
    if (key < node->key) {
      node = node->left;
    } else {
      node = node->right;
    }
  }
  return -1;
}

/* Rotations:
        y    right->   x
       / \   <-left   / \
      x   c          a   y
     / \                / \
    a   b              b   c
*/
static void rot_right (iBiNode_s **np) {
  iBiNode_s *x;
  iBiNode_s *y;
  iBiNode_s *tmp;
  y = *np;
  if (!y) return;
  x = y->left;
  if (!x) return;
  tmp = x->right;
  x->right = y;
  y->left = tmp;
  *np = x;
}

static void rot_left (iBiNode_s **np) {
  iBiNode_s *x;
  iBiNode_s *y;
  iBiNode_s *tmp;
  x = *np;
  if (!x) return;
  y = x->right;
  if (!y) return;
  tmp = y->left;
  y->left = x;
  x->right = tmp;
  *np = y;
}

static iBiNode_s *ibi_new (u64 key) {
  iBiNode_s *node = ezalloc(sizeof(*node));
  node->key = key;
  return node;
}

#if 0
static void insert (iBiNode_s **np, Rec_s r) {
  iBiNode_s *node = *np;
  if (*np) {
    if (cmplump(r.key, node->rec.key) < 0) {
      insert(&node->left, r);
    } else {
      insert(&node->right, r);
    }
  } else {
    *np = ibi_new(r);
  }
}

int ibi_insert (iBiNode_s **root, Rec_s r) {
  insert(&t->root, r);
  return 0;
}

static void delete_node (iBiNode_s **np) {
  iBiNode_s *node = *np;
  if (!node->right) {
    *np = node->left;
    free(node);
    return;
  }
  if (!node->left) {
    *np = node->right;
    free(node);
    return;
  }
  rot_right(np);
  delete_node(&((*np)->right));
}

static void delete (iBiNode_s **np, u64 key) {
  iBiNode_s *node = *np;
  if (!node) fatal("Key not found");
  if (key < node->key) {
    delete(&node->left, key);
  } else if (key > node->key) {
    delete(&node->right, key);
  } else {
    delete_node(np);
  }
}

int ibi_delete (iBiNode_s **root, u64 key) {
  delete(&t->root, key);
  return 0;
}
#endif

int ibi_insert (iBiTree_s *tree, u64 key) {
  iBiNode_s **np = &tree->root;
  for (;;) {
    iBiNode_s *node = *np;
    if (!node) break;
    if (key < node->key) {
      np = &node->left;
    } else {
      np = &node->right;
    }
  }
  *np = ibi_new(key);
  return 0;
}

static void delete_node (iBiNode_s **np) {
  static int odd = 0;
  iBiNode_s *node;
  for (;;) {
    node = *np;
    if (!node->right) {
      *np = node->left;
      break;
    }
    if (!node->left) {
      *np = node->right;
      break;
    }
    if (odd & 1) {
      rot_left(np);
      np = &((*np)->left);
    } else {
      rot_right(np);
      np = &((*np)->right);
    }
    ++odd;
  }
  free(node);
}

int ibi_delete (iBiTree_s *tree, u64 key) {
  iBiNode_s **np = &tree->root;
  for (;;) {
    iBiNode_s *node = *np;
    if (!node) fatal("Key not found");
    if (key == node->key) break;
    if (key < node->key) {
      np = &node->left;
    } else {
      np = &node->right;
    }
  }
  delete_node(np);
  return 0;
}
