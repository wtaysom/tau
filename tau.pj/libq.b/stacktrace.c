/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that
 * can be found in the LICENSE file.
 */
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

void stacktrace(void)
{
	enum { MAX_DEPTH = 20 };
	void	*trace[MAX_DEPTH];
	char	**messages = (char **)NULL;
	int	trace_size = 0;
	int	i;

	trace_size = backtrace(trace, MAX_DEPTH);
	messages = backtrace_symbols(trace, trace_size);
	fprintf(stderr, "Stack back trace:\n");
	for (i = 0; i < trace_size; i++) {
		fprintf(stderr, "\t%s\n", messages[i]);
	}
	fprintf(stderr, "\n");
	free(messages);
}
