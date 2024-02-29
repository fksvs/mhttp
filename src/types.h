#ifndef MHTTP_TYPES_H
#define MHTTP_TYPES_H

#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include "list.h"

#define SERVER_NAME "mhttp"
#define LISTEN_ADDRESS_IPV4 "127.0.0.1"
#define LISTEN_ADDRESS_IPV6 "::ffff:127.0.0.1"
#define LISTEN_PORT 8000
#define DEFAULT_IPV6 false

#define WORKING_DIR "./"
#define DEFAULT_LOG_FILE "/var/log/mhttp"

#define DEFAULT_TLS false
#define CERT_FILE "cert.pem"
#define KEY_FILE "key.pem"

#define BACKLOG 10
#define MAX_EVENTS 100
#define BUFF_SIZE 16384

#define MAX_METHOD_LEN 10
#define MAX_DIR_LEN 256
#define MAX_VER_LEN 10

#define MAX_HEADERS 20
#define MAX_HEADER_NAME 50
#define MAX_HEADER_VALUE 50

#define MAX_EXTENSION_LEN 10
#define MAX_MIME_TYPE_LEN 50
#define MAX_EXTENSIONS 100

struct server_t {
	bool use_ipv6;
	bool use_tls;
	int serverfd;
	int epollfd;
	int listen_port;
	char listen_address[INET6_ADDRSTRLEN];
	char working_dir[MAX_DIR_LEN];
	char log_file[MAX_DIR_LEN];
	char cert_file[MAX_DIR_LEN];
	char key_file[MAX_DIR_LEN];
	SSL_CTX *ctx;
	list_t *client_list;
};

struct client_t {
	int sockfd;
	int client_port;
	char client_addr[INET6_ADDRSTRLEN];
	SSL *ssl;
};

struct http_header {
	char name[MAX_HEADER_NAME];
	char value[MAX_HEADER_VALUE];
};

struct http_request {
	char method[MAX_METHOD_LEN];
	char uri[MAX_DIR_LEN];
	char version[MAX_VER_LEN];
	struct http_header headers[MAX_HEADERS];
};

struct mime_entry {
	char extension[MAX_EXTENSION_LEN];
	char mime_type[MAX_MIME_TYPE_LEN];
};

#endif
