#include <stdio.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "types.h"
#include "response.h"

int parse_request(struct server_t *server, struct client_t *client,
		char *request_buffer, int total, struct http_request *request)
{
	char *token, *rest;

	token = strtok_r(request_buffer, "\r\n", &rest);
	if (token == NULL) {
		send_error(server, client, 400, "Bad Request");
		return -1;
	}

	if (sscanf(token, "%s %s %s", request->method, request->uri,
		   request->version) == EOF) {
		send_error(server, client, 500, "Internal Server Error");
		return -1;
	}

	return 0;
}

int check_request(struct server_t *server, struct client_t *client,
		struct http_request *request)
{
	if (strncmp(request->method, "GET", MAX_METHOD_LEN) &&
	    strncmp(request->method, "HEAD", MAX_METHOD_LEN)) {
		send_error(server, client, 501, "Not Implemented");
		return -1;
	}
	if (strncmp(request->version, "HTTP/1.1", MAX_VER_LEN)) {
		send_error(server, client, 505, "Version Not Supported");
		return -1;
	}
	if (strstr(request->uri, "..")) {
		send_error(server, client, 403, "Forbidden");
		return -1;
	}

	return 0;
}

int process_request(struct server_t *server, struct client_t *client)
{
	struct http_request request;
	char request_buffer[BUFF_SIZE];
	int total;

	if (server->use_tls)
		total = SSL_read(client->ssl, request_buffer, BUFF_SIZE);
	else
		total = recv(client->sockfd, request_buffer, BUFF_SIZE, 0);

	if (total == 0 || total == -1) {
		send_error(server, client, 500, "Internal Server Error");
		return -1;
	}

	if (parse_request(server, client, request_buffer, total, &request) == -1)
		return -1;
	if (check_request(server, client, &request) == -1)
		return -1;
	if (send_response(server, client, &request) == -1)
		return -1;

	return 0;
}
