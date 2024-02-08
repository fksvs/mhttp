#ifndef MHTTP_RESPONSE_H
#define MHTTP_RESPONSE_H

#include "types.h"

void send_error(struct client_t *client, int err_code, char *reason);
void get_filetype(char *uri, char *filetype);
int send_response(struct server_t *server, struct client_t *client,
		struct http_request *request);

#endif
