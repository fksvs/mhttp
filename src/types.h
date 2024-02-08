#ifndef MHTTP_TYPES_H
#define MHTTP_TYPES_H

#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/ssl.h>

#define SERVER_NAME "mhttp"
#define LISTEN_ADDRESS "127.0.0.1"
#define LISTEN_PORT 443
#define WORKING_DIR "./"

#define DEFAULT_TLS false
#define CERT_FILE "cert.pem"
#define KEY_FILE "key.pem"

#define BACKLOG 10
#define MAX_EVENTS 100
#define BUFF_SIZE 16384

#define MAX_METHOD_LEN 10
#define MAX_DIR_LEN 256
#define MAX_VER_LEN 10

#define MAX_EXTENSION_LEN 10
#define MAX_MIME_TYPE_LEN 50
#define MAX_EXTENSIONS 100

struct node_t {
	void *data;
	struct node_t *prev;
	struct node_t *next;
};

typedef struct {
	unsigned int list_size;
	void (*destroy)(void *data);
	struct node_t *head;
	struct node_t *tail;
} list_t;

struct server_t {
	int serverfd;
	int epollfd;
	int listen_port;
	char listen_address[INET_ADDRSTRLEN];
	char working_dir[MAX_DIR_LEN];
	bool use_tls;
	char cert_file[MAX_DIR_LEN];
	char key_file[MAX_DIR_LEN];
	SSL_CTX *ctx;
	list_t *client_list;
};

struct client_t {
	int sockfd;
	int client_port;
	char client_addr[INET_ADDRSTRLEN];
	SSL *ssl;
};

struct http_request {
	char method[MAX_METHOD_LEN];
	char uri[MAX_DIR_LEN];
	char version[MAX_VER_LEN];
};

struct mime_entry {
	char extension[MAX_EXTENSION_LEN];
	char mime_type[MAX_MIME_TYPE_LEN];
};

#endif
