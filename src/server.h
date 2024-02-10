#ifndef MHTTP_SERVER_H
#define MHTTP_SERVER_H

#include <sys/socket.h>
#include "types.h"

int init_client(struct server_t *server, int sockfd, struct sockaddr_storage *addr);
void destroy_client(void *data);
void close_client(struct server_t *server, struct node_t *node);
void mhttp_listener(struct server_t *server);

#endif
