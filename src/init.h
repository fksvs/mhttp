#ifndef MHTTP_INIT_H
#define MHTTP_INIT_H

#include "types.h"

void init_signal(void (*signal_exit)(int));
void init_tls(struct server_t *server);
void init_epoll(struct server_t *server);
void init_socket(struct server_t *server);
void init_log(struct server_t *server);

#endif
