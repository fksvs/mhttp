#ifndef MHTTP_INIT
#define MHTTP_INIT

#include "types.h"

void init_signal(void (*signal_exit)(int));
void init_tls(struct server_t *server);
void init_epoll(struct server_t *server);
void init_socket(struct server_t *server);
void init_log();

#endif
