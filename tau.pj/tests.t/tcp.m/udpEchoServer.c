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

#include <style.h>
#include <eprintf.h>

enum { ECHOMAX = 255 };

int main (int argc, char *argv[])
{
	int			sock;
	struct sockaddr_in	echoServAddr;	// Local address
	struct sockaddr_in	echoClntAddr;	// Client address
	u16			echoServPort;	// Server port
	socklen_t		cliAddrLen;	// Length of client address
	int	recvMsgSize;
	char	echoBuffer[ECHOMAX];
	int	rc;

	setprogname(argv[0]);

	if (argc != 2) {
		pr_usage("<UDP Server Port>");
	}
	echoServPort = atoi(argv[1]);

	/* Create socket for sending/receiving datagrams */
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) fatal("socket():");

	/* Construct local address structure */
	zero(echoServAddr);
	echoServAddr.sin_family      = AF_INET;		// Internet address family
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);// Any incoming interface
	echoServAddr.sin_port        = htons(echoServPort);// Local port

	/* Bind to the local address */
	rc = bind(sock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr));
	if (rc < 0) fatal("bind():");

	for (;;) {
		/* Set the size of the in-out parameter */
		cliAddrLen = sizeof(echoClntAddr);

		/* Block until receive message from a client */
		recvMsgSize = recvfrom(sock, echoBuffer, ECHOMAX, 0,
				(struct sockaddr *)&echoClntAddr, &cliAddrLen);
		if (recvMsgSize < 0) fatal("recvfrom():");

		printf("Handling cleint %s\n", inet_ntoa(echoClntAddr.sin_addr));

		rc = sendto(sock, echoBuffer, recvMsgSize, 0,
			(struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr));
		if (rc != recvMsgSize) {
			fatal("sendto() sendt a different number of bytes than expected");
		}
	}
}
