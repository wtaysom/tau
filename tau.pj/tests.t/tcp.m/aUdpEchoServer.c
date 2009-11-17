/*
 * "TCP/IP Sockets in C," Micahel J. Donahoo and Kenneth L. Calvert,
 * Morgan Kaufmann Publishers, 2001, pp 39-40.
 * Adapted by Paul Taysom.
 */
#include <stdio.h>
#include <sys/socket.h>	// For socket(), connect(), send(), and recv()
#include <arpa/inet.h>	// For sockaddr_in and inet_addr()
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>	// for O_NONBLOCK and FASYNC
#include <signal.h>
#include <errno.h>

#include <style.h>
#include <eprintf.h>

enum { ECHOMAX = 255 };

int	Sock;

void UseIdleTime (void)
{
	printf(".\n");
	sleep(3);
}

void SIGIOHandler (int signalType)
{
	struct sockaddr_in	echoClntAddr;	// Address of datagram source
	unsigned int		clntLen;	// Address length
	char	echoBuffer[ECHOMAX];
	int	recvMsgSize;
	int	rc;

	do {
		clntLen = sizeof(echoClntAddr);

		recvMsgSize = recvfrom(Sock, echoBuffer, ECHOMAX, 0,
				(struct sockaddr *)&echoClntAddr, &clntLen);
		if (recvMsgSize < 0) {
			/* Only acceptable error: recvfrom() would have blocked */
			if (errno != EWOULDBLOCK) {
				fatal("recvfrom() failed:");
			}
		} else {
			printf("Handling cleint %s\n",
				inet_ntoa(echoClntAddr.sin_addr));

			rc = sendto(Sock, echoBuffer, recvMsgSize, 0,
					(struct sockaddr *)&echoClntAddr,
					sizeof(echoClntAddr));
			if (rc != recvMsgSize) fatal("sendto():");
		}
	} while (recvMsgSize >= 0);
}

int main (int argc, char *argv[])
{
	struct sockaddr_in	echoServAddr;	// Local address
	u16			echoServPort;	// Server port
	struct sigaction	handler;
	int	rc;

	setprogname(argv[0]);

	if (argc != 2) {
		pr_usage("<UDP Server Port>");
	}
	echoServPort = atoi(argv[1]);

	/* Create socket for sending/receiving datagrams */
	Sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (Sock < 0) fatal("socket():");

	/* Construct local address structure */
	zero(echoServAddr);
	echoServAddr.sin_family      = AF_INET;		// Internet address family
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);// Any incoming interface
	echoServAddr.sin_port        = htons(echoServPort);// Local port

	/* Bind to the local address */
	rc = bind(Sock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr));
	if (rc < 0) fatal("bind():");

	/* Set signal handler for SIGIO */
	handler.sa_handler = SIGIOHandler;
	/* Create mask that masks all signals */
	if (sigfillset( &handler.sa_mask) < 0) fatal("sigfillset():");
	handler.sa_flags = 0;

	/* Set signal handling for itnerrupt signals */
	if (sigaction(SIGIO, &handler, 0) < 0) fatal("sigaction():");

	/* We must own the socket to receive the SIGIO message */
	if (fcntl(Sock, F_SETOWN, getpid()) < 0) {
		fatal("Unable to set process owner to us:");
	}
	/* Arrange for nonblocking I/O and SIGIO delviery */
	if (fcntl(Sock, F_SETFL, O_NONBLOCK | FASYNC) < 0) {
		fatal("Unable to put client sock into nonblocking/async mode:");
	}

	for (;;) {
		UseIdleTime();
	}
}
