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
#include "types.h"
#include "request.h"

int init_client(struct server_t *server, int sockfd, struct sockaddr_in *addr)
{
        struct epoll_event ev;
        struct client_t *client = malloc(sizeof(struct client_t));
        char ip_addr[INET_ADDRSTRLEN];
        SSL *ssl;

        ssl = SSL_new(server->ctx);
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

        if (epoll_ctl(server->epollfd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
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
                                struct client_t *client =
                                        (struct client_t *)events[n].data.ptr;
                                close(client->sockfd);
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
                                struct client_t *client =
                                        (struct client_t *)events[n].data.ptr;
                                if (process_request(server, client) == -1)
                                        close_client(client);
                        }
                }
        }
}
