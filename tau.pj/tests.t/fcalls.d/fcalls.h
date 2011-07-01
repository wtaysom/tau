/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */

#ifndef _FCALLS_H_
#define _FCALLS_H_ 1

#ifndef _SYTLE_H_
#include <style.h>
#endif

#ifndef _EPRINTF_H_
#include <eprintf.h>
#endif

/*
 * Define macros to make it easier to test file system related system calls.
 * The main goal, is to wrapper each system call so that it when an error
 * occurs, it is easy to track back the case that fails.
 *
 * Yes, this is a shameless abuse of macros but it makes the tests easier
 * to read and write.
 */

#define HERE		MYFILE, __FUNCTION__, __LINE__
#define HERE_PRM	const char *file, const char *function, int line

#define chdir(_p)		chdirq(HERE, _p)
#define chdirErr(_err, _p)	chdirE(HERE, _err, _p)

int chdirq(HERE_PRM, const char *path);
int chdirE(HERE_PRM, int err, const char *path);


#endif
