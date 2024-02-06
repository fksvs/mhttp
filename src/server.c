#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "server.h"
#include "types.h"
#include "list.h"
#include "request.h"

int init_client(struct server_t *server, int sockfd, struct sockaddr_in *addr)
{
	struct epoll_event ev;
	struct client_t *client = malloc(sizeof(struct client_t));
	struct node_t *node;
	char ip_addr[INET_ADDRSTRLEN];
	SSL *ssl;

	if (server->use_tls) {
		ssl = SSL_new(server->ctx);
		SSL_set_fd(ssl, sockfd);
		client->ssl = ssl;

		if (SSL_accept(ssl) <= 0) {
			SSL_free(ssl);
			free(client);
			return -1;
		}
	} else {
		client->ssl = NULL;
	}

	client->sockfd = sockfd;
	memcpy(&client->addr, addr, sizeof(struct sockaddr_in));

	node = list_insert_data(server->client_list, (void *)client);

	ev.events = EPOLLIN;
	ev.data.ptr = node;

	if (epoll_ctl(server->epollfd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
		if (server->use_tls) {
			SSL_shutdown(ssl);
			SSL_free(ssl);
		}
		free(client);
		return -1;
	}

	inet_ntop(AF_INET, &addr->sin_addr, ip_addr, INET_ADDRSTRLEN);
	syslog(LOG_INFO, "%s:%d connected\n", ip_addr, addr->sin_port);

	return 0;
}

void destroy_client(void *data)
{
	struct client_t *client = (struct client_t *)data;
	free(client);
}

void close_client(struct server_t *server, struct node_t *node)
{
	char ip_addr[INET_ADDRSTRLEN];
	struct client_t *client = (struct client_t *)node->data;

	inet_ntop(AF_INET, &client->addr.sin_addr, ip_addr, INET_ADDRSTRLEN);
	syslog(LOG_INFO, "%s:%d disconnected\n", ip_addr,
	       client->addr.sin_port);

	if (client->ssl) {
		SSL_shutdown(client->ssl);
		SSL_free(client->ssl);
	}
	close(client->sockfd);
	list_remove_data(server->client_list, node);
}

void mhttp_listener(struct server_t *server)
{
	struct epoll_event events[MAX_EVENTS];
	socklen_t len = sizeof(struct sockaddr_in);

	for (;;) {
		int nfds = epoll_wait(server->epollfd, events, MAX_EVENTS, -1);
		if (nfds == -1) {
			syslog(LOG_ERR, "epoll_wait() error : %s",
			       strerror(errno));
			exit(EXIT_FAILURE);
		}

		for (int n = 0; n < nfds; n++) {
			if ((events[n].events & EPOLLERR) ||
			    (events[n].events & EPOLLHUP) ||
			    (events[n].events & EPOLLRDHUP)) {
				struct node_t *node = (struct node_t *)events[n].data.ptr;
				close_client(server, node);
				continue;
			}

			if (events[n].data.fd == server->serverfd) {
				struct sockaddr_in addr;
				int clientfd = accept(server->serverfd,
						      (struct sockaddr *)&addr,
						      &len);
				if (clientfd == -1)
					continue;

				if (init_client(server, clientfd, &addr) == -1) {
					close(clientfd);
					continue;
				}
			} else {
				struct node_t *node = (struct node_t *)events[n].data.ptr;
				struct client_t *client = (struct client_t *)node->data;
				if (process_request(server, client) == -1)
					close_client(server, node);
			}
		}
	}
}
