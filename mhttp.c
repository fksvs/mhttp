#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <syslog.h>
#include <sys/resource.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define SYSLOG_IDENT "mhttp"
#define LISTEN_ADDRESS "127.0.0.1"
#define LISTEN_PORT 443
#define WORKING_DIR "./"

#define CERT_FILE "cert.pem"
#define KEY_FILE "key.pem"

#define BACKLOG 10
#define MAX_EVENTS 100
#define BUFF_SIZE 16384

#define MAX_METHOD_LEN 10
#define MAX_DIR_LEN 256
#define MAX_VER_LEN 10
#define MAX_HEADERS 10
#define MAX_HEADER_NAME 50
#define MAX_HEADER_VALUE 100
#define MAX_FILETYPE_LEN 50

struct server_t {
	int serverfd;
	int epollfd;
	int listen_port;
	char listen_address[INET_ADDRSTRLEN];
	char working_dir[MAX_DIR_LEN];
	char cert_file[MAX_DIR_LEN];
	char key_file[MAX_DIR_LEN];
	SSL_CTX *ctx;
} server;

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

void send_error(struct client_t *client, int error_code, char *reason)
{
	char response[BUFF_SIZE];

	snprintf(response, BUFF_SIZE, "HTTP/1.1 %d %s\r\n\
Server: mhttp\r\nConnection: close\r\n\r\n", error_code, reason);

	SSL_write(client->ssl, response, strlen(response));
}

int parse_request(struct client_t *client, char *request_buffer, int total,
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

	return 0;
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

int send_response(struct client_t *client, struct http_request *request)
{
	FILE *fp;
	char complete_path[MAX_DIR_LEN * 2], filetype[MAX_FILETYPE_LEN];
	char response_header[BUFF_SIZE];
	struct stat file_stat;

	snprintf(complete_path, MAX_DIR_LEN * 2, "%s%s", server.working_dir,
		 request->uri);
	if (strstr(request->uri, "..")) {
		send_error(client, 403, "Forbidden");
		return -1;
	}

	get_filetype(request->uri, filetype);

	if ((fp = fopen(complete_path, "r")) == NULL) {
		send_error(client, 404, "Not Found");
		return -1;
	}
	if (stat(complete_path, &file_stat) == -1) {
		send_error(client, 500, "Internal Server Error");
		return -1;
	}

	snprintf(response_header, BUFF_SIZE, "HTTP/1.1 200 OK\r\nServer: mhttp\r\n\
Content-Type: %s\r\nContent-Length: %ld\r\n\r\n", filetype, file_stat.st_size);
	if (SSL_write(client->ssl, response_header, strlen(response_header)) == -1)
		return -1;

	if (!strncmp(request->method, "GET", MAX_METHOD_LEN)) {
		char response_body[BUFF_SIZE];
		int total;
		while ((total = fread(response_body, 1, BUFF_SIZE, fp)) > 0) {
			if (SSL_write(client->ssl, response_body, total) == -1)
				return -1;
			memset(response_body, 0, BUFF_SIZE);
		}
	}

	fclose(fp);
	return 0;
}

int process_request(struct client_t *client)
{
	struct http_request request;
	char request_buffer[BUFF_SIZE];
	int total;

	total = SSL_read(client->ssl, request_buffer, BUFF_SIZE);
	if (total == 0 || total == -1) {
		send_error(client, 500, "Internal Server Error");
		return -1;
	}

	if (parse_request(client, request_buffer, total, &request) == -1)
		return -1;
	if (check_request(client, &request) == -1)
		return -1;
	if (send_response(client, &request) == -1)
		return -1;

	return 0;
}

int init_client(int sockfd, struct sockaddr_in *addr)
{
	struct epoll_event ev;
	struct client_t *client = malloc(sizeof(struct client_t));
	char ip_addr[INET_ADDRSTRLEN];
	SSL *ssl;

	ssl = SSL_new(server.ctx);
        SSL_set_fd(ssl, sockfd);
	client->ssl = ssl;

        if (SSL_accept(ssl) <= 0) {
		SSL_free(ssl);
		free(client);
                return -1;
	}

	client->sockfd = sockfd;
	memcpy(&client->addr, addr, sizeof(struct sockaddr_in));

	ev.events = EPOLLIN;
	ev.data.ptr = client;

	if (epoll_ctl(server.epollfd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
		SSL_shutdown(ssl);
		SSL_free(ssl);
		free(client);
		return -1;
	}

	inet_ntop(AF_INET, &addr->sin_addr, ip_addr, INET_ADDRSTRLEN);
	syslog(LOG_INFO, "%s:%d connected\n", ip_addr, addr->sin_port);

	return 0;
}

void close_client(struct client_t *client)
{
	char ip_addr[INET_ADDRSTRLEN];

	inet_ntop(AF_INET, &client->addr.sin_addr, ip_addr, INET_ADDRSTRLEN);
	syslog(LOG_INFO, "%s:%d disconnected\n", ip_addr,
	       client->addr.sin_port);

	SSL_shutdown(client->ssl);
	SSL_free(client->ssl);
	close(client->sockfd);
	free(client);
}

void mhttp_listener()
{
	struct epoll_event events[MAX_EVENTS];
	socklen_t len = sizeof(struct sockaddr_in);

	for (;;) {
		int nfds = epoll_wait(server.epollfd, events, MAX_EVENTS, -1);
		if (nfds == -1) {
			syslog(LOG_ERR, "epoll_wait() error : %s",
			       strerror(errno));
			exit(EXIT_FAILURE);
		}

		for (int n = 0; n < nfds; n++) {
			if ((events[n].events & EPOLLERR) ||
			    (events[n].events & EPOLLHUP) ||
			    (events[n].events & EPOLLRDHUP)) {
				struct client_t *client =
					(struct client_t *)events[n].data.ptr;
				close(client->sockfd);
				continue;
			}

			if (events[n].data.fd == server.serverfd) {
				struct sockaddr_in addr;
				int clientfd = accept(server.serverfd,
						      (struct sockaddr *)&addr,
						      &len);
				if (clientfd == -1)
					continue;

				if (init_client(clientfd, &addr) == -1) {
					close(clientfd);
					continue;
				}
			} else {
				struct client_t *client =
					(struct client_t *)events[n].data.ptr;
				if (process_request(client) == -1)
					close_client(client);
			}
		}
	}
}

void daemonize_server()
{
	int fd;
	struct sigaction act;
	struct rlimit rlim;

	if (getrlimit(RLIMIT_NOFILE, &rlim) == -1)
		exit(EXIT_FAILURE);

	for (unsigned long i = 3; i < rlim.rlim_max; i++)
		close(i);

	act.sa_handler = SIG_DFL;
	for (unsigned long i = 0; i < _NSIG; i++)
		sigaction(i, &act, NULL);

	switch (fork()) {
	case -1:
		exit(EXIT_FAILURE);
	case 0:
		break;
	default:
		exit(EXIT_SUCCESS);
	}

	if (setsid() == -1)
		exit(EXIT_FAILURE);

	switch (fork()) {
	case -1:
		exit(EXIT_FAILURE);
	case 0:
		break;
	default:
		exit(EXIT_SUCCESS);
	}

	for (int i = 0; i < 3; i++)
		close(i);

	if ((fd = open("/dev/null", O_RDWR)) == -1)
		exit(EXIT_FAILURE);
	if (dup2(fd, 1) != 1)
		exit(EXIT_FAILURE);
	if (dup2(fd, 2) != 2)
		exit(EXIT_FAILURE);

	umask(0);
	if (chdir(server.working_dir) == -1)
		exit(EXIT_FAILURE);
}

void signal_exit(int signum)
{
	struct rlimit rlim;

	syslog(LOG_INFO, "closing %s, signal = %d", SYSLOG_IDENT, signum);
	closelog();

	if (getrlimit(RLIMIT_NOFILE, &rlim) == -1)
		exit(EXIT_FAILURE);

	for (unsigned long i = 0; i < rlim.rlim_max; i++)
		close(i);

	SSL_CTX_free(server.ctx);
	exit(EXIT_SUCCESS);
}

void init_signal()
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = &signal_exit;

	if (sigaction(SIGINT, &act, NULL) == -1) {
		syslog(LOG_ERR, "sigaction() [SIGINT] error : %s",
		       strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (sigaction(SIGTERM, &act, NULL) == -1) {
		syslog(LOG_ERR, "sigaction() [SIGTERM] error : %s",
		       strerror(errno));
		exit(EXIT_FAILURE);
	}

	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &act, NULL) == -1) {
		syslog(LOG_ERR, "sigaction() [SIGPIPE] error: %s",
		       strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void init_socket()
{
	struct sockaddr_in addr;
	int yes = 1;

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(server.listen_port);
	inet_pton(AF_INET, server.listen_address, &addr.sin_addr.s_addr);

	if ((server.serverfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		syslog(LOG_ERR, "socket() error : %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (setsockopt(server.serverfd, SOL_SOCKET, SO_REUSEADDR, &yes,
		       sizeof(int)) == -1) {
		syslog(LOG_ERR, "setsockopt() error : %s", strerror(errno));
		close(server.serverfd);
		exit(EXIT_FAILURE);
	}
	if (bind(server.serverfd, (struct sockaddr *)&addr,
		 sizeof(struct sockaddr_in)) == -1) {
		syslog(LOG_ERR, "bind() error : %s", strerror(errno));
		close(server.serverfd);
		exit(EXIT_FAILURE);
	}
	if (listen(server.serverfd, BACKLOG) == -1) {
		syslog(LOG_ERR, "listen() error : %s", strerror(errno));
		close(server.serverfd);
		exit(EXIT_FAILURE);
	}
}

void init_tls()
{
	const SSL_METHOD *method;
	SSL_CTX *ctx;

	method = TLS_server_method();

	ctx = SSL_CTX_new(method);
	if (!ctx) {
		syslog(LOG_ERR, "SSL_CTX_new() error");
		exit(EXIT_FAILURE);
	}

	if (SSL_CTX_use_certificate_file(ctx, server.cert_file, SSL_FILETYPE_PEM) <= 0) {
		printf("hata\n");
		syslog(LOG_ERR, "SSL_CTX_use_certificate_file() error");
		exit(EXIT_FAILURE);
	}
	if(SSL_CTX_use_PrivateKey_file(ctx, server.key_file, SSL_FILETYPE_PEM) <= 0) {
		syslog(LOG_ERR, "SSL_CTX_use_PrivateKey_file() error");
		exit(EXIT_FAILURE);
	}

	server.ctx = ctx;
}

void init_epoll()
{
	struct epoll_event ev;

	if ((server.epollfd = epoll_create1(0)) == -1) {
		syslog(LOG_ERR, "epoll_create1() error : %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	ev.events = EPOLLIN;
	ev.data.fd = server.serverfd;
	if (epoll_ctl(server.epollfd, EPOLL_CTL_ADD, server.serverfd, &ev) == -1) {
		syslog(LOG_ERR, "epoll_ctl() error : %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void mhttp_usage()
{
	fprintf(stdout, "\n usage: ./mhttp [options]\n\n options:\n\
\t-a [listen address] : listen address for incoming connection\n\
\t-p [listen port] : listen port for incoming connections\n\
\t-d [directory] : main directory to serve\n\
\t-c [certificate file] : TLS certificate file\n\
\t-k [key file] : TLS private key file\n\
\t-h : this help message\n\n");
}

void arg_parser(int argc, char *argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "a:p:d:c:k:h")) != -1) {
		switch (opt) {
		case 'a':
			strncpy(server.listen_address, optarg, INET_ADDRSTRLEN);
			break;
		case 'p':
			server.listen_port = atoi(optarg);
			break;
		case 'd':
			strncpy(server.working_dir, optarg, MAX_DIR_LEN);
			break;
		case 'c':
			strncpy(server.cert_file, optarg, MAX_DIR_LEN);
			break;
		case 'k':
			strncpy(server.key_file, optarg, MAX_DIR_LEN);
			break;
		case 'h':
			mhttp_usage();
			exit(EXIT_SUCCESS);
		}
	}
}

int main(int argc, char *argv[])
{
	if (getuid()) {
		fprintf(stderr, "permission denied.\n");
		exit(EXIT_FAILURE);
	}

	server.listen_port = LISTEN_PORT;
	strncpy(server.listen_address, LISTEN_ADDRESS, INET_ADDRSTRLEN);
	strncpy(server.working_dir, WORKING_DIR, MAX_DIR_LEN);
	strncpy(server.cert_file, CERT_FILE, MAX_DIR_LEN);
	strncpy(server.key_file, KEY_FILE, MAX_DIR_LEN);

	if (argc > 1)
		arg_parser(argc, argv);

	daemonize_server();
	openlog(SYSLOG_IDENT, LOG_NDELAY | LOG_PID, LOG_USER);
	init_signal();
	init_socket();
	init_tls();
	init_epoll();
	mhttp_listener();

	for (;;)
		pause();

	exit(EXIT_SUCCESS);
}
