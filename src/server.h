#ifndef MHTTP_SERVER
#define MHTTP_SERVER

#include <sys/socket.h>
#include "types.h"

int init_client(struct server_t *server, int sockfd, struct sockaddr_in *addr);
void destroy_client(void *data);
void close_client(struct server_t *server, struct node_t *node);
void mhttp_listener(struct server_t *server);

#endif
