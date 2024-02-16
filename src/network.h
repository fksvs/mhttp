#ifndef MHTTP_NETWORK_H
#define MHTTP_NETWORK_H

#include <sys/socket.h>
#include "types.h"

int send_data(struct client_t *client, char *data, size_t size);
int recv_data(struct client_t *client, char *data, size_t buff_size);

#endif
