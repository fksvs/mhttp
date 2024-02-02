#ifndef MHTTP_TYPES
#define MHTTP_TYPES

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
#define MAX_FILETYPE_LEN 50

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
};

struct client_t {
	int sockfd;
	struct sockaddr_in addr;
	SSL *ssl;
};

struct http_request {
	char method[MAX_METHOD_LEN];
	char uri[MAX_DIR_LEN];
	char version[MAX_VER_LEN];
};

#endif
