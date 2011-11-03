/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <token.h>

int main (int argc, char *argv[])
{
	char	*t;
	int	id = -1;

	open_token(
		"/sys/kernel/debug/tracing/events"
		"/raw_syscalls/sys_enter/format",
		" \n\t");
	for (;;) {
		t = get_token();
		if (!t) break;
		printf("%s\n", t);
	}
	open_token(
		"/sys/kernel/debug/tracing/events"
		"/raw_syscalls/sys_enter/format",
		" \n\t");
	for (;;) {
		t = get_token();
		if (!t) break;
		if (strcmp(t, "ID:") == 0) {
			t = get_token();
			if (!t) break;
			id = atoi(t);
		}
	}
	printf("id = %d\n", id);
	return 0;
}
