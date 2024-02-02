#ifndef MHTTP_UTILS
#define MHTTP_UTILS

#include "types.h"

void daemonize_server();
void mhttp_usage();
void arg_parser(struct server_t *server, int argc, char *argv[]);

#endif
