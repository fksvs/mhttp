#ifndef MHTTP_RESPONSE
#define MHTTP_RESPONSE

#include "types.h"

void send_error(struct client_t *client, int err_code, char *reason);
void get_filetype(char *uri, char *filetype);
int send_response(struct server_t *server, struct client_t *client,
		  struct http_request *request);

#endif
