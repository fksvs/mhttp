#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "response.h"
#include "types.h"

static struct mime_entry mime_dict[] = {
	{".txt", "text/plain"},
	{".html", "text/html"},
	{".css", "text/css"},
	{".xml", "application/xml"},
	{".bin", "application/octet-stream"},
	{".js", "text/javascript"},
	{".json", "application/json"},
	{".jpg", "image/jpg"},
	{".jpeg", "image/jpeg"},
	{".png", "image/png"},
	{".svg", "image/svg+xml"},
	{".gif", "image/gif"},
	{".tar", "application/x-tar"}
};

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

static int get_filetype(char *uri, char *filetype) {
	char *extension = strrchr(uri, '.');
	int num_items = sizeof(mime_dict) / sizeof(struct mime_entry) - 1;

	if (extension != NULL && extension != uri) {
		for (int i = 0; i < num_items; i++) {
			if (!strncmp(mime_dict[i].extension, extension,
					MAX_EXTENSION_LEN)) {
				strncpy(filetype, mime_dict[i].mime_type,
					MAX_MIME_TYPE_LEN);
				return 0;
			}
		}
	}

	strncpy(filetype, "application/octet-stream", MAX_MIME_TYPE_LEN);
	return 0;
}

int send_response(struct server_t *server, struct client_t *client,
		struct http_request *request)
{
	FILE *fp;
	char complete_path[MAX_DIR_LEN * 2], filetype[MAX_MIME_TYPE_LEN];
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
