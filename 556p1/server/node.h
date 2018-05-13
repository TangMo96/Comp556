#ifndef _NODE_H
#define _NODE_H


#include <stdlib.h>
#include <netinet/in.h>


#define IS_SENDING 1
#define IS_RECEIVING 2


/* to maintain application information related to a connected socket */
struct node {
	int fd;

	struct sockaddr_in client_addr;

	int operation_type;                    /* 1 - send, 2 - receive */

	char *message;                         /* cached message */
	size_t pending_message;                /* remaining data to receive/send (in bytes) */
	size_t processed_message;              /* data received/sent (in bytes) */

	struct node *next;
};

/* remove a node */
void dump(struct node *head, int fd);

/* add a node */
void add(struct node *head, int fd, struct sockaddr_in addr);


#endif
