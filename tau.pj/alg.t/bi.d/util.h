/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _UTIL_H_
#define _UTIL_H_ 1

#include <lump.h>

char *rndstring(unint n);
Lump_s rnd_lump(void);
Lump_s fixed_lump(unint n);
Lump_s seq_lump(void);

#endif
