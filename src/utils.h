#ifndef MHTTP_UTILS_H
#define MHTTP_UTILS_H

#include "types.h"

void daemonize_server();
void mhttp_usage();
void arg_parser(struct server_t *server, int argc, char *argv[]);

#endif
