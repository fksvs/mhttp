#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/resource.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "types.h"
#include "list.h"
#include "utils.h"
#include "init.h"
#include "request.h"
#include "response.h"
#include "server.h"
#include "log.h"

struct server_t server;

void signal_exit(int signum)
{
	struct rlimit rlim;

	log_info("closing %s, signal = %d", SERVER_NAME, signum);
	close_log_files();

	if (getrlimit(RLIMIT_NOFILE, &rlim) == -1)
		exit(EXIT_FAILURE);

	for (unsigned long i = 0; i < rlim.rlim_max; i++)
		close(i);

	SSL_CTX_free(server.ctx);
	list_destroy(server.client_list);
	exit(EXIT_SUCCESS);
}

void init_server_struct()
{
	server.use_ipv6 = DEFAULT_IPV6;
	server.listen_port = LISTEN_PORT;

	if (server.use_ipv6)
		strncpy(server.listen_address, LISTEN_ADDRESS_IPV6, INET6_ADDRSTRLEN);
	else
		strncpy(server.listen_address, LISTEN_ADDRESS_IPV4, INET_ADDRSTRLEN);

	strncpy(server.working_dir, WORKING_DIR, MAX_DIR_LEN);
	strncpy(server.log_file, DEFAULT_LOG_FILE, MAX_DIR_LEN);

	server.use_tls = DEFAULT_TLS;
	strncpy(server.cert_file, CERT_FILE, MAX_DIR_LEN);
	strncpy(server.key_file, KEY_FILE, MAX_DIR_LEN);

	server.client_list = init_list(&destroy_client);
}

int main(int argc, char *argv[])
{
	if (getuid()) {
		log_info("permission denied.\n");
		exit(EXIT_FAILURE);
	}

	init_server_struct();

	if (argc > 1)
		arg_parser(&server, argc, argv);

#ifndef DEBUG
	daemonize_server();
#endif
	init_log(&server);
	init_signal(&signal_exit);
	init_socket(&server);

	if (server.use_tls)
		init_tls(&server);

	init_epoll(&server);
	log_info("%s started. listening on %s:%d", SERVER_NAME, 
		server.listen_address, server.listen_port);

	mhttp_listener(&server);

	exit(EXIT_SUCCESS);
}
