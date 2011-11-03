/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef _TOKEN_H_
#define _TOKEN_H_ 1

enum { MAX_TOKEN = 1024 };

void close_token(void);
void open_token(char *file, char *delim);
char *get_token(void);
int get_delimiter(void);

#endif
