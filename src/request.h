#ifndef MHTTP_REQUEST
#define MHTTP_REQUEST

#include "types.h"

int parse_request(struct client_t *client, char *request_buffer,
		int total, struct http_request *request);
int check_reqeust(struct client_t *client, struct http_request *request);
int process_request(struct server_t *server, struct client_t *client);

#endif
