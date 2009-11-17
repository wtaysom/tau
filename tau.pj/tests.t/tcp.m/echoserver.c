/*
 * "TCP/IP Sockets in C," Micahel J. Donahoo and Kenneth L. Calvert,
 * Morgan Kaufmann Publishers, 2001, pp 19-20.
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

enum { RCVBUFSIZE = 32, MAXPENDING = 5 };

void HandleTCPclient (int clntSocket)
{
	char	echoBuffer[RCVBUFSIZE];
	int	recvMsgSize;
	int	rc;

	/* Receive message from client */
	recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0);
	if (recvMsgSize < 0) fatal("recv():");

	/* Send received string and receive again until end of transmission */
	while (recvMsgSize > 0) {	// zero indicates end of treansmission
		/* Echo message back to client */
		rc = send(clntSocket, echoBuffer, recvMsgSize, 0);
		if (rc != recvMsgSize) fatal("send():");

		/* See if there is more data to receive */
		recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0);
		if (recvMsgSize < 0) fatal("recv():");
	}
	close(clntSocket);
}

int main (int argc, char *argv[])
{
	int			servSock;	// Socket descriptor for server
	int			clntSock;	// Socket descriptor for client
	struct sockaddr_in	echoServAddr;	// Local address
	struct sockaddr_in	echoClntAddr;	// Client address
	u16			echoServPort;	// Server port
	unsigned int		clntLen;	// Length of client address
	int			rc;

	setprogname(argv[0]);

	if (argc != 2) {
		pr_usage("<Server Port>");
	}
	echoServPort = atoi(argv[1]);

	/* Create socket for incoming connections */
	servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (servSock < 0) fatal("socket():");

	/* Construct local address structure */
	zero(echoServAddr);
	echoServAddr.sin_family      = AF_INET;		// Internet address family
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);// Any incoming interface
	echoServAddr.sin_port        = htons(echoServPort);// Local port

	/* Bind to the local address */
	rc = bind(servSock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr));
	if (rc < 0) fatal("bind():");

	/* Mark the socket so it will listen for incoming connections */
	rc = listen(servSock, MAXPENDING);
	if (rc < 0) fatal("listen():");

	for (;;) {
		/* Set the size of the in-out parameter */
		clntLen = sizeof(echoClntAddr);

		/* Wait for a client to connect */
		clntSock = accept(servSock, (struct sockaddr *)&echoClntAddr, &clntLen);
		if (clntSock < 0) fatal("accept():");

		/* clntSock is connected to a client! */
		printf("Handling cleint %s\n", inet_ntoa(echoClntAddr.sin_addr));

		HandleTCPclient(clntSock);
	}
}
