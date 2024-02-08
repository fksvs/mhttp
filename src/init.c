#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "init.h"
#include "types.h"
#include "request.h"

void init_signal(void (*signal_exit)(int))
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = signal_exit;

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

void init_tls(struct server_t *server)
{
	const SSL_METHOD *method;
	SSL_CTX *ctx;

	method = TLS_server_method();

	ctx = SSL_CTX_new(method);
	if (!ctx) {
		syslog(LOG_ERR, "SSL_CTX_new() error");
		exit(EXIT_FAILURE);
	}

	if (SSL_CTX_use_certificate_file(ctx, server->cert_file,
					 SSL_FILETYPE_PEM) <= 0) {
		syslog(LOG_ERR, "SSL_CTX_use_certificate_file() error");
		exit(EXIT_FAILURE);
	}
	if (SSL_CTX_use_PrivateKey_file(ctx, server->key_file,
					SSL_FILETYPE_PEM) <= 0) {
		syslog(LOG_ERR, "SSL_CTX_use_PrivateKey_file() error");
		exit(EXIT_FAILURE);
	}

	server->ctx = ctx;
}

void init_epoll(struct server_t *server)
{
	struct epoll_event ev;

	if ((server->epollfd = epoll_create1(0)) == -1) {
		syslog(LOG_ERR, "epoll_create1() error : %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	ev.events = EPOLLIN;
	ev.data.fd = server->serverfd;
	if (epoll_ctl(server->epollfd, EPOLL_CTL_ADD, server->serverfd, &ev) == -1) {
		syslog(LOG_ERR, "epoll_ctl() error : %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void init_socket(struct server_t *server)
{
	struct sockaddr_in addr;
	int yes = 1;

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(server->listen_port);
	inet_pton(AF_INET, server->listen_address, &addr.sin_addr.s_addr);

	if ((server->serverfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		syslog(LOG_ERR, "socket() error : %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (setsockopt(server->serverfd, SOL_SOCKET, SO_REUSEADDR, &yes,
		       sizeof(int)) == -1) {
		syslog(LOG_ERR, "setsockopt() error : %s", strerror(errno));
		close(server->serverfd);
		exit(EXIT_FAILURE);
	}
	if (bind(server->serverfd, (struct sockaddr *)&addr,
		 sizeof(struct sockaddr_in)) == -1) {
		syslog(LOG_ERR, "bind() error : %s", strerror(errno));
		close(server->serverfd);
		exit(EXIT_FAILURE);
	}
	if (listen(server->serverfd, BACKLOG) == -1) {
		syslog(LOG_ERR, "listen() error : %s", strerror(errno));
		close(server->serverfd);
		exit(EXIT_FAILURE);
	}
}

void init_log()
{
	openlog(SERVER_NAME, LOG_NDELAY | LOG_PID, LOG_USER);
}
