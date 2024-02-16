#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "types.h"
#include "network.h"
#include "response.h"

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

	send_data(client, response, strlen(response));
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

static int head_method(struct client_t *client, char *filetype, size_t size)
{
	char response_header[BUFF_SIZE];

	snprintf(response_header, BUFF_SIZE, "HTTP/1.1 200 OK\r\nServer: %s\r\n\
Content-Type: %s\r\nContent-Length: %ld\r\n\r\n", SERVER_NAME, filetype, size);

	if (send_data(client, response_header, strlen(response_header)) <= 0)
		return -1;

	return 0;
}

static int get_method(struct client_t *client, char *path, char *filetype, size_t size)
{
	FILE *fp;
	char buffer[BUFF_SIZE];
	int total;

	if ((fp = fopen(path, "r")) == NULL) {
		send_error(client, 404, "Not Found");
		return -1;
	}

	if (head_method(client, filetype, size) < 0)
		return -1;

	while ((total = fread(buffer, 1, BUFF_SIZE, fp)) > 0) {
		if (send_data(client, buffer, total) <= 0) {
			fclose(fp);
			return -1;
		}

		memset(buffer, 0, BUFF_SIZE);
	}

	fclose(fp);
	return 0;
}

static int options_method(struct client_t *client)
{
	char response_header[BUFF_SIZE];

	snprintf(response_header, BUFF_SIZE, "HTTP/1.1 200 OK\r\nServer: %s\r\n\
Allow: GET, HEAD, OPTIONS\r\nConnection: close\r\n\r\n", SERVER_NAME);

	if (send_data(client, response_header, strlen(response_header)) <= 0)
		return -1;

	return 0;
}

static void get_complete_path(char *request_uri, char *working_dir, char *path)
{
	char *uri_ptr = request_uri;

	if (request_uri[0] == '/')
		uri_ptr = request_uri + 1;

	snprintf(path, MAX_DIR_LEN * 2, "%s%s", working_dir, uri_ptr);
}

static size_t get_file_size(struct client_t *client, char *path)
{
	struct stat file_stat;

	if (stat(path, &file_stat) == -1) {
		int err = errno;
		switch (err) {
		case ENOENT:
			send_error(client, 404, "Not Found");
			return -1;
		default:
			send_error(client, 500, "Internal Server Error");
			return -1;
		}
	}

	return file_stat.st_size;
}

int send_response(struct server_t *server, struct client_t *client,
		struct http_request *request)
{
	char complete_path[MAX_DIR_LEN * 2], filetype[MAX_MIME_TYPE_LEN];
	size_t file_size;

	get_complete_path(request->uri, server->working_dir, complete_path);
	get_filetype(request->uri, filetype);
	if ((file_size = get_file_size(client, complete_path)) == (size_t)-1)
			return -1;

	if (!strncmp(request->method, "GET", MAX_METHOD_LEN))
		return get_method(client, complete_path, filetype, file_size);
	if (!strncmp(request->method, "HEAD", MAX_METHOD_LEN))
		return head_method(client, filetype, file_size);
	if (!strncmp(request->method, "OPTIONS", MAX_METHOD_LEN))
		return options_method(client);

	return 0;
}
