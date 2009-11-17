/*
 * "TCP/IP Sockets in C," Micahel J. Donahoo and Kenneth L. Calvert,
 * Morgan Kaufmann Publishers, 2001, pp 13-15.
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

enum { RCVBUFSIZE = 32 };

int main (int argc, char *argv[])
{
	int			sock;		// Socket descriptor
	struct sockaddr_in	echoServAddr;	// Echo server address
	u16			echoServPort;	// Echo server port
	char			*servIP;	// Server IP address (dotted quad)
	char	*echoString;
	char	echoBuffer[RCVBUFSIZE];
	unint	echoStringLen;
	int	bytesRcvd, totalBytesRcvd;	// Bytes read in single recv()
						// and total bytes read
	int	rc;

	setprogname(argv[0]);

	if ((argc < 3) || (argc >4)) {
		pr_usage("<Server IP> <Echo Word> [<Echo Port>]");
	}
	servIP = argv[1];
	echoString = argv[2];

	if (argc == 4) {
		echoServPort = atoi(argv[3]);
	} else {
		echoServPort = 7;	// well-known port for echo services
	}

	/* Create a reliable, stream socket using TCP */
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock< 0) fatal("socket():");

	/* Construct the server address structure */
	zero(echoServAddr);
	echoServAddr.sin_family      = AF_INET;
	echoServAddr.sin_addr.s_addr = inet_addr(servIP);
	echoServAddr.sin_port        = htons(echoServPort);

	/* Establish the connection to the echo server */
	rc = connect(sock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr));
	if (rc < 0) fatal("connect():");

	echoStringLen = strlen(echoString);

	/* Send the string to the server */
	rc = send(sock, echoString, echoStringLen, 0);
	if (rc != echoStringLen) {
		fatal("send() sent a different number of bytes than expected:");
	}
	/* Receive the same string back from the server */
	totalBytesRcvd = 0;
	printf("Received: ");
	while (totalBytesRcvd < echoStringLen) {
		/* Receive up to the buffer size (minus 1 to leave space for
		 * a null terminator) bytes from the sender
		 */
		bytesRcvd = recv(sock, echoBuffer, RCVBUFSIZE-1, 0);
		if (bytesRcvd <= 0) {
			fatal("recv() failed of connection closed prematurely:");
		}
		totalBytesRcvd += bytesRcvd;
		echoBuffer[bytesRcvd] = '\0';
		printf(echoBuffer);
	}
	printf("\n");
	close(sock);
	return 0;
}
