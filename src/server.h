#ifndef MHTTP_SERVER
#define MHTTP_SERVER

#include <sys/socket.h>
#include "types.h"

int init_client(int sockfd, struct sockaddr_in *addr);
void close_client(struct server_t *server, struct client_t *client);
void mhttp_listener(struct server_t *server);

#endif
