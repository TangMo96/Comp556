#ifndef _COMMON_H
#define _COMMON_H


#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <asm/errno.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include "node.h"


#define DEBUG 1

#define MAX_MESSAGE_LENGTH 65535      /* the max length of each message */

#define MAX_CONNECTION 5              /* the max number of acceptable connections */

#define TIMEOUT_TIME 10000            /* timeout time for select */

#define MODE_PING_PONG 1
#define MODE_WEB_SERVER 2


/* do select */
int select_fds(int sock, fd_set *read_set, fd_set *write_set, struct node *head);

/* create a new socket on a given port */
int create_sock(uint16_t port);

/* create a new fd on the socket, and return new new fd */
int create_fd(int sock, struct node *head);

/* send message to a fd */
void send_message(struct node *current);

/* recv message from a fd */
int recv_message(struct node *head, struct node *current, char *recv_buf, int mode);

/* extract file_path from a message */
int extract_file_path(char *message, size_t m_length, char *file_path);

/* read a file to res and return the length */
int read_file(char *path, char *res);

/* check whether a file exists, or whether it is a folder */
int check_existence_and_folder(char *path);

/* generate HTTP response */
void generate_header(struct node *current, char *title);

#endif
