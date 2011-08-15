/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _BUF_H_
#define _BUF_H_ 1

#ifndef _STYLE_H_
#include <style.h>
#endif

typedef struct Cache_s Cache_s;

typedef struct Buf_s {
  Cache_s *cache;
  u64 block;
  int inuse;
  bool dirty;
  bool clock;
  void *user;
  void *d;
} Buf_s;

static inline void buf_dirty(Buf_s *b)
{
  b->dirty = TRUE;
}

Cache_s *cache_new(char *filename, u64 num_bufs, u64 blockSize);
bool     cache_balanced(Cache_s *cache);
Buf_s   *buf_new(Cache_s *cache);
void     buf_free(Buf_s **bp);
Buf_s   *buf_get(Cache_s *cache, u64 block);
Buf_s   *buf_scratch(Cache_s *cache);
void     buf_put(Buf_s **bp);
void     buf_toss(Buf_s *b);

#endif
