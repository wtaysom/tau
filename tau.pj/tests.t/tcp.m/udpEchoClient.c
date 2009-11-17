/*
 * "TCP/IP Sockets in C," Micahel J. Donahoo and Kenneth L. Calvert,
 * Morgan Kaufmann Publishers, 2001, pp 36-38.
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
	int			sock;		// Socket descriptor
	struct sockaddr_in	echoServAddr;	// Echo server address
	struct sockaddr_in	fromAddr;	// Source address of echo
	u16			echoServPort;	// Echo server port
	socklen_t		fromSize;	// address size for recvfrom()
	char			*servIP;	// Server IP address (dotted quad)
	char	*echoString;
	char	echoBuffer[ECHOMAX+1];
	unint	echoStringLen;
	unint	respStringLen;
	int	rc;

	setprogname(argv[0]);

	if ((argc < 3) || (argc >4)) {
		pr_usage("<Server IP> <Echo Word> [<Echo Port>]");
	}
	servIP = argv[1];
	echoString = argv[2];

	echoStringLen = strlen(echoString);
	if (echoStringLen > ECHOMAX) {
		fatal("Echo word too long %ld", echoStringLen);
	}
	if (argc == 4) {
		echoServPort = atoi(argv[3]);
	} else {
		echoServPort = 7;	// well-known port for echo services
	}

	/* Create a datagram/UPD socket */
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock< 0) fatal("socket():");

	/* Construct the server address structure */
	zero(echoServAddr);
	echoServAddr.sin_family      = AF_INET;
	echoServAddr.sin_addr.s_addr = inet_addr(servIP);
	echoServAddr.sin_port        = htons(echoServPort);

	/* Send the string to the server */
	rc = sendto(sock, echoString, echoStringLen, 0,
		(struct sockaddr *)&echoServAddr, sizeof(echoServAddr));
	if (rc != echoStringLen) {
		fatal("sendto() sent a different number of bytes than expected:");
	}

	/* Recv a response */
	fromSize = sizeof(fromAddr);
	respStringLen = recvfrom(sock, echoBuffer, ECHOMAX, 0,
				(struct sockaddr *)&fromAddr, &fromSize);
	if (respStringLen != echoStringLen) fatal("recfrom() failed:");

	if (echoServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr) {
		fatal("received a packet from unknown source.");
	}
	echoBuffer[respStringLen] = '\0';
	printf("Received: %s\n", echoBuffer);

	close(sock);
	return 0;
}
