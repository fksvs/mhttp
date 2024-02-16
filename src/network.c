#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "types.h"
#include "network.h"

int send_data(struct client_t *client, char *data, size_t size)
{
	int total;

	if (client->ssl) {
		total = SSL_write(client->ssl, data, size);
	} else {
		total = send(client->sockfd, data, size, 0);
	}

	if (total <= 0)
		return -1;

	return total;
}

int recv_data(struct client_t *client, char *data, size_t buff_size)
{
	int total;

	if (client->ssl) {
		total = SSL_read(client->ssl, data, buff_size);
	} else {
		total = recv(client->sockfd, data, buff_size, 0);
	}

	if (total <= 0)
		return -1;

	return total;
}
