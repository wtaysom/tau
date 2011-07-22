/*
 * Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

#ifndef _TEST_H_
#define _TEST_H_

#include <style.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int testError(const char *what);
#define test(_e_)	((void)((_e_) || testError(WHERE " (" # _e_ ")")))

#ifdef __cplusplus
}
#endif

#endif
