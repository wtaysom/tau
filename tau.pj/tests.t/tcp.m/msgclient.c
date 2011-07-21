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
#include <uuid/uuid.h>

#include <style.h>
#include <eprintf.h>
#include <mystdlib.h>
#include <q.h>
#include <debug.h>
#include <tau/msg.h>
#include <tau/net.h>
#include "msgtcp.h"

enum {	NUM_BUCKETS = 37,
	MAX_KEYS    = 20 };

typedef struct process_s	process_s;
typedef struct gate_s		gate_s;

struct gate_s {
	void		*gt_next;
	void		*gt_tag;
	process_s	*gt_avatar;
	u64		gt_id;
	u8		gt_type;
	u8		gt_reserverd[7];
};

struct process_s {
	char	pr_name[TAU_NAME];
	gate_s	*pr_gates[NUM_BUCKETS];
	key_s	pr_keys[MAX_KEYS];
	key_s	*pr_top;
	d_q	pr_msgq;
	guid_t	pr_id;
};

process_s	Me;
char		*ServerIP = "127.0.0.1";

struct sockaddr_in	TcpServAddr;	// Echo server address

#define PROMPT	prompt(FN_ARG)

static void prompt (const char *m, int line)
{
	if (m) printf("%s<%d>? ", m, line);
	else printf("continue? ");
	getchar();
}

void init_tcp (char *servIP)
{
	u16	tcpServPort;	// Echo server port

	tcpServPort = PORT_MSGTCP;

	/* Construct the server address structure */
	zero(TcpServAddr);
	TcpServAddr.sin_family      = AF_INET;
	TcpServAddr.sin_addr.s_addr = inet_addr(servIP);
	TcpServAddr.sin_port        = htons(tcpServPort);

	uuid_generate(Me.pr_id);
}

int send_tcp (msgbuf_s *mb)
{
	int	sock;		// Socket descriptor
	int	rc;

		/* Create a reliable, stream socket using TCP */
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock< 0) fatal("socket():");

		/* Establish the connection to the echo server */
	rc = connect(sock, (struct sockaddr *)&TcpServAddr, sizeof(TcpServAddr));
	if (rc < 0) fatal("connect():");

		/* Send the msgbuf to the server */
	mb->mb_type = SEND_MSGTCP;
	uuid_copy(mb->mb_pr_id, Me.pr_id);
PRd(sizeof(*mb));
	rc = send(sock, mb, sizeof(*mb), 0);
	if (rc != sizeof(*mb)) {
		fatal("send() sent a different number of bytes than expected:");
	}
PROMPT;
	close(sock);
	return 0;
}

int receive_tcp (msgbuf_s *mb)
{
	int	sock;				// Socket descriptor
	int	bytesRcvd;
	int	remainder;
	int	rc;
	u8	*buf = (u8 *)mb;

PROMPT;
		/* Create a reliable, stream socket using TCP */
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock< 0) fatal("socket():");

		/* Establish the connection to the echo server */
	rc = connect(sock, (struct sockaddr *)&TcpServAddr, sizeof(TcpServAddr));
	if (rc < 0) fatal("connect():");

	mb->mb_type = SEND_MSGTCP;
	uuid_copy(mb->mb_pr_id, Me.pr_id);	// Don't need to send it all
	rc = send(sock, mb, sizeof(*mb), 0);
	if (rc != sizeof(*mb)) {
		fatal("send() sent a different number of bytes than expected:");
	}
	for (remainder = sizeof(*mb); remainder; remainder -= bytesRcvd) {
			/* Receive up to the buffer size bytes from the sender */
		bytesRcvd = recv(sock, buf, remainder, 0);
PRd(bytesRcvd);
		if (bytesRcvd <= 0) {
			fatal("recv() failed of connection closed prematurely:");
		}
		buf += bytesRcvd;
	}
	close(sock);
	return 0;
}

static gate_s *hash (u64 id)
{
	unint	h = id % NUM_BUCKETS;

	return (gate_s *)&Me.pr_gates[h];
}

static gate_s *find_gate (u64 id)
{
	gate_s	*prev;
	gate_s	*next;

	prev = hash(id);
	next = prev->gt_next;
	while (next && (next->gt_id != id)) {
		next = next->gt_next;
	}
	return next;
}

static void add_gate (gate_s *gate)
{
	gate_s	*prev;

	prev = hash(gate->gt_id);
	gate->gt_next = prev->gt_next;
	prev->gt_next = gate;
}

static gate_s *delete_gate (u64 id)
{
	gate_s	*prev;
	gate_s	*next;

	prev = hash(id);
	next = prev->gt_next;
	while (next) {
		if (next->gt_id == id) {
			prev->gt_next = next->gt_next;
			break;
		}
		prev = next;
		next = next->gt_next;
	}
	return next;
}

static unint add_key (key_s key)
{
	key_s	*k;

	k = Me.pr_top;
	if (!k) return 0;
	Me.pr_top = *(key_s **)k;
	*k = key;
	return k - Me.pr_keys;
}

static key_s get_key (unint index)
{
	if (index >= MAX_KEYS) return Me.pr_keys[0];
	return Me.pr_keys[index];
}

static void rmv_key (unint index)
{
	key_s	*key;

	if (index >= MAX_KEYS) return;
	key = &Me.pr_keys[index];
	if (!key->k_id) return;
	key->k_id = 0;
	*(key_s **)key = Me.pr_top;
	Me.pr_top = key;
}

int init_msg_tau (const char *name)
{
	key_s	*key;
	unint	i;

	init_tcp(ServerIP);

	strlcpy(Me.pr_name, name, TAU_NAME);

	for (i = MAX_KEYS; i > 0; i--) {
		key = &Me.pr_keys[i];
		*(key_s **)key = Me.pr_top;
		Me.pr_top = key;
	}
	init_dq( &Me.pr_msgq);
	return 0;
}

int create_gate_tau (void *msg)
{
	msg_s		*m = msg;
	static u64	id = 0;
	gate_s		*gate;
	key_s		key;
	ki_t		k;

	gate = ezalloc(sizeof(*gate));
	gate->gt_id      = ++id;
	gate->gt_tag     = m->q.q_tag;
	gate->gt_type    = m->q.q_type;
	gate->gt_avatar = &Me;
	add_gate(gate);
	m->cr_id = gate->gt_id;
	key.k_length   = 0;
	key.k_node     = 0;
	key.k_type     = gate->gt_type;
	key.k_reserved = 0;
	key.k_id       = gate->gt_id;
	k = add_key(key);
	if (!k) fatal("Out of keys");
	m->q.q_passed_key = k;

	return 0;
}

int receive_tau (void *msg)
{
	msg_s		*m = msg;
	msgbuf_s	*mb;
	packet_s	*p;
	ki_t		k;
	int		rc;

	mb = emalloc(sizeof(msgbuf_s));
	receive_tcp(mb);
//	deq_dq( &Me.pr_msgq, mb, mb_process);
//	if (!mb) return ENOMSGS;

	p = &mb->mb_packet;

	rc = p->pk_error;
	if (rc) return rc;

	k = 0;
	if (p->pk_passed_key.k_id) {
		k = add_key(p->pk_passed_key);
		if (!k) fatal("Out of keys");
	}
	p->pk_sys.q_passed_key = k;
	memmove(m, &p->pk_sys, sizeof(msg_s));
	free(mb);

	return 0;
}

int send_tau (ki_t k, void *msg)
{
	msg_s		*m = msg;
	msgbuf_s	*mb;
	packet_s	*p;
	key_s		key;
	gate_s		*gate;
	process_s	*avatar;

	key = get_key(k);
	if (!key.k_id) return EBADKEY;
	if (key.k_type & ONCE) rmv_key(k);

	mb = emalloc(sizeof(msgbuf_s));

	memmove( &mb->mb_body, &m->b, sizeof(mb->mb_body));

	p = &mb->mb_packet;

	gate = find_gate(key.k_id);
	if (!gate) {
		/* Broken gate */
		free(mb);
		return 0;
	}
	p->pk_sys.q_tag = gate->gt_tag;
	p->pk_sys.q_passed_key = 0;
	avatar = gate->gt_avatar;

	if (gate->gt_type & ONCE) delete_gate(gate->gt_id);

	send_tcp(mb);
	free(mb);

	//enq_dq( &avatar->pr_msgq, mb, mb_process);
	return 0;
}

ki_t make_gate (
	void		*tag,
	unsigned	type)
{
	msg_s	m;
	int	rc;

	m.q.q_tag = tag;
	m.q.q_type = type & ~(READ_DATA | WRITE_DATA);
	rc = create_gate_tau( &m);
	if (rc) {
		fatal("create_gate %d", rc);
	}
	return m.q.q_passed_key;
}

int main (int argc, char *argv[])
{
	msg_s	m;
	ki_t	key;
	int	rc;

	setprogname(argv[0]);

	if (argc > 1) {
		ServerIP = argv[1];
	}

	init_msg_tau(argv[0]);

	key = make_gate(main, RESOURCE);

	rc = send_tau(key, &m);
	if (rc) fatal("send %d", rc);

	rc = receive_tau( &m);
	if (rc) fatal("receive %d", rc);

	return 0;
}
