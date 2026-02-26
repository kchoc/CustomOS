#ifndef SOCKET_H
#define SOCKET_H

#include <list.h>

typedef enum sock_type {
	SOCK_TYPE_STREAM,
	SOCK_TYPE_DGRAM,
	SOCK_TYPE_RAW,
	// ...
} sock_type_t;

typedef enum sock_state {
	SOCK_STATE_CLOSED,
	SOCK_STATE_OPEN,
	SOCK_STATE_LISTENING,
	SOCK_STATE_CONNECTED,
	// ...
} sock_state_t;

typedef struct socket {
	sock_type_t     type;
	sock_state_t 	state;
	int 			backlog;
	list_t 			accept_queue;
	list_t 			peer_queue;
	list_t 			read_queue;
	list_t 			write_queue;

	void*           private; // protocol-specific data
} socket_t;

#endif // SOCKET_H
