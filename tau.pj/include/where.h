/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* WHERE defines where in the code you are executing. It returns a
 * structure that has the file, function, and line number.
 */
#ifndef _WHERE_H_
#define _WHERE_H_ 1

#include <style.h>  /* Get old definition of WHERE */

#undef WHERE

typedef struct Where_s {
  const char *file;
  const char *fn;
  int line;
} Where_s;

#define WHERE ({ Where_s w = { __FILE__, __FUNCTION__, __LINE__ }; w; })

#endif
