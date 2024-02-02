#ifndef MHTTP_INIT
#define MHTTP_INIT

#include "types.h"

void sinal_exit(int signum);
void init_signal();
void init_tls(struct server_t *server);
void init_epoll(struct server_t *server);
void init_socket(struct server_t *server);
void init_log();

#endif
