#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "types.h"

void send_error(struct client_t *client, int err_code, char *reason)
{
	char response[BUFF_SIZE];

	snprintf(response, BUFF_SIZE, "HTTP/1.1 %d %s\r\n\
Server: %s\r\nConnection: close\r\n\r\n",
		 err_code, reason, SERVER_NAME);

	if (client->ssl)
		SSL_write(client->ssl, response, strlen(response));
	else
		send(client->sockfd, response, strlen(response), 0);
}

void get_filetype(char *uri, char *filetype)
{
	if (strstr(uri, ".html"))
		strncpy(filetype, "text/html", MAX_FILETYPE_LEN);
	else if (strstr(uri, ".css"))
		strncpy(filetype, "text/css", MAX_FILETYPE_LEN);
	else if (strstr(uri, ".gif"))
		strncpy(filetype, "image/gif", MAX_FILETYPE_LEN);
	else if (strstr(uri, ".jpg") || strstr(uri, ".jpeg"))
		strncpy(filetype, "image/jpg", MAX_FILETYPE_LEN);
	else if (strstr(uri, ".js"))
		strncpy(filetype, "text/javascript", MAX_FILETYPE_LEN);
	else if (strstr(uri, ".xml"))
		strncpy(filetype, "text/xml", MAX_FILETYPE_LEN);
	else
		strncpy(filetype, "text/plain", MAX_FILETYPE_LEN);
}

int send_response(struct server_t *server, struct client_t *client,
		struct http_request *request)
{
	FILE *fp;
	char complete_path[MAX_DIR_LEN * 2], filetype[MAX_FILETYPE_LEN];
	char response_header[BUFF_SIZE];
	struct stat file_stat;

	snprintf(complete_path, MAX_DIR_LEN * 2, "%s%s", server->working_dir,
		 request->uri);

	get_filetype(request->uri, filetype);

	if ((fp = fopen(complete_path, "r")) == NULL) {
		send_error(client, 404, "Not Found");
		return -1;
	}
	if (stat(complete_path, &file_stat) == -1) {
		send_error(client, 500, "Internal Server Error");
		return -1;
	}

	snprintf(response_header, BUFF_SIZE, "HTTP/1.1 200 OK\r\nServer: %s\r\n\
Content-Type: %s\r\nContent-Length: %ld\r\n\r\n",
		 SERVER_NAME, filetype, file_stat.st_size);

	if (client->ssl) {
		if (SSL_write(client->ssl, response_header,
				strlen(response_header)) == -1)
			return -1;
	} else {
		if (send(client->sockfd, response_header,
				strlen(response_header), 0) == -1)
			return -1;
	}

	if (!strncmp(request->method, "GET", MAX_METHOD_LEN)) {
		char response_body[BUFF_SIZE];
		int total;
		while ((total = fread(response_body, 1, BUFF_SIZE, fp)) > 0) {
			if (client->ssl) {
				if (SSL_write(client->ssl, response_body, total) == -1)
					return -1;
			} else {
				if (send(client->sockfd, response_body, total, 0) == -1)
					return -1;
			}
			memset(response_body, 0, BUFF_SIZE);
		}
	}

	fclose(fp);
	return 0;
}
