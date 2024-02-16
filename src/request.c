#include <stdio.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "request.h"
#include "types.h"
#include "network.h"
#include "response.h"

int parse_request(struct client_t *client, char *request_buffer,
		struct http_request *request)
{
	char *token, *rest;

	token = strtok_r(request_buffer, "\r\n", &rest);
	if (token == NULL) {
		send_error(client, 400, "Bad Request");
		return -1;
	}

	if (sscanf(token, "%s %s %s", request->method, request->uri,
		   request->version) == EOF) {
		send_error(client, 500, "Internal Server Error");
		return -1;
	}

	return 0;
}

int check_request(struct client_t *client, struct http_request *request)
{
	if (strncmp(request->method, "GET", MAX_METHOD_LEN) &&
	    strncmp(request->method, "HEAD", MAX_METHOD_LEN)) {
		send_error(client, 501, "Not Implemented");
		return -1;
	}
	if (strncmp(request->version, "HTTP/1.1", MAX_VER_LEN)) {
		send_error(client, 505, "Version Not Supported");
		return -1;
	}
	if (strstr(request->uri, "../") || strstr(request->uri, "%2e%2e%2f") ||
		strstr(request->uri, "%2e%2e/") || strstr(request->uri, "..%2f")) {
		send_error(client, 403, "Forbidden");
		return -1;
	}

	return 0;
}

int process_request(struct server_t *server, struct client_t *client)
{
	struct http_request request;
	char request_buffer[BUFF_SIZE];

	if (recv_data(client, request_buffer, BUFF_SIZE) <= 0) {
		send_error(client, 500, "Internal Server Error");
		return -1;
	}

	if (parse_request(client, request_buffer, &request) == -1)
		return -1;
	if (check_request(client, &request) == -1)
		return -1;
	if (send_response(server, client, &request) == -1)
		return -1;

	return 0;
}
