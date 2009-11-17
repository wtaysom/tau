#ifndef _MSGTCP_H_
#define _MSGTCP_H_ 1

#ifndef _TAU_MSG_H_
#include <tau/msg.h>
#endif

#ifndef _TAU_NET_H_
#include <tau/net.h>
#endif

#ifndef _Q_H_
#include <q.h>
#endif

enum { PORT_MSGTCP = 6151,
	 INIT_MSGTCP = 1, SEND_MSGTCP, RECV_MSGTCP };

typedef struct msgbuf_s		msgbuf_s;
struct msgbuf_s { //XXX: tcpbuf_s?
	dqlink_t	mb_process;
	u64		mb_type;
	guid_t		mb_pr_id;
	packet_s	mb_packet;
	body_u		mb_body;
};

#endif
