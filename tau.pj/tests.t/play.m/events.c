/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <debug.h>
#include <eprintf.h>
#include <mystdlib.h>
#include <puny.h>
#include <style.h>

enum {	EVENT_SHIFT = 4,
	EVENT_MAX = 1 << EVENT_SHIFT,
	EVENT_MASK = EVENT_MAX - 1};

enum {	EVENT_INIT = 1,
	EVENT_ADD,
	EVENT_DETACH };

struct History_s {
	u64		events;
	pthread_mutex_t	lock;
};

#define HISTORY_INITIALIZER { 0, PTHREAD_MUTEX_INITIALIZER }

static void set_event (struct History_s *history, unsigned event)
{
	pthread_mutex_lock(&history->lock);
	history->events = (history->events << EVENT_SHIFT) | event;
	pthread_mutex_unlock(&history->lock);
}

void print_history (struct History_s *history, int max_name, char *event_name[max_name])
{
	u64	events;
	u64	event;
	int	i;

	pthread_mutex_lock(&history->lock);
	events = history->events;
	pthread_mutex_unlock(&history->lock);
	for (i = 1; events; i++, events >>= EVENT_SHIFT) {
		event = events & EVENT_MASK;
		if (event < max_name) {
			printf("%2d. %s\n", i, event_name[event]);
		} else {
			printf("%2d. (unknown) %llx\n", i, event);
		}
	}
}
	
struct arg_s {
	char	name[20];
	int	event;
};

struct History_s History;

void *history_test (void *arg)
{
	struct arg_s	*a = arg;
	struct timespec	sleep = { 0, 3 * ONE_THOUSAND };
	int	i;

	sleep.tv_sec = 0;
	for (i = 0; i < Option.iterations; i++) {
		set_event( &History, a->event);
		sleep.tv_nsec = 3 * ONE_THOUSAND + urand(10000);
		nanosleep(&sleep, NULL);
	}
	return NULL;
}

void start_threads (unsigned threads)
{
	pthread_t	*thread;
	unsigned	i;
	int		rc;
	struct arg_s	*arg;
	struct arg_s	*a;

	thread = ezalloc(threads * sizeof(pthread_t));
	arg    = ezalloc(threads * sizeof(struct arg_s));
	for (i = 0, a = arg; i < threads; i++, a++) {
		sprintf(a->name, "thead%d", i);
		a->event = i + 1;
		rc = pthread_create( &thread[i], NULL, history_test, a);
		if (rc) {
			eprintf("pthread_create %d\n", rc);
			break;
		}
	}
	while (i--) {
		pthread_join(thread[i], NULL);
	}
}

char *Event_name[] = {
	"<nil>",
	"init",
	"add",
	"detach" };

int main (int argc, char *argv[])
{
	punyopt(argc, argv, NULL, NULL);
	start_threads(Option.numthreads);
	print_history(&History, sizeof(Event_name) / sizeof(char *), Event_name);
	return 0;
}

