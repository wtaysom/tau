/*
 * "TCP/IP Sockets in C," Micahel J. Donahoo and Kenneth L. Calvert,
 * Morgan Kaufmann Publishers, 2001, p 48.
 * Adapted by Paul Taysom.
 */
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <style.h>
#include <eprintf.h>

void InterruptSignalHandler (int signalType)
{
	printf("Interrupt Received.\n");// Exiting program.\n");
	//exit(1);
	sleep(3);
}

int main (int argc, char *argv[])
{
	struct sigaction	handler;

	handler.sa_handler = InterruptSignalHandler;
	/* Create mask that masks all signals */
	if (sigfillset( &handler.sa_mask) < 0) fatal("sigfillset():");

	handler.sa_flags = 0;

	/* Set signal handling for itnerrupt signals */
	if (sigaction(SIGINT, &handler, 0) < 0) fatal("sigaction():");

	for (;;) {
		pause();
	}
}
