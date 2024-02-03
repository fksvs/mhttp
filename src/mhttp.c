#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <netinet/in.h>
#include <sys/resource.h>
#include "types.h"
#include "list.h"
#include "utils.h"
#include "init.h"
#include "request.h"
#include "response.h"
#include "server.h"

struct server_t server;

void signal_exit(int signum)
{
	struct rlimit rlim;

	syslog(LOG_INFO, "closing %s, signal = %d", SERVER_NAME, signum);
	closelog();

	if (getrlimit(RLIMIT_NOFILE, &rlim) == -1)
		exit(EXIT_FAILURE);

	for (unsigned long i = 0; i < rlim.rlim_max; i++)
		close(i);

	SSL_CTX_free(server.ctx);
	list_destroy(server.client_list);
	exit(EXIT_SUCCESS);
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
	server.use_tls = DEFAULT_TLS;
	strncpy(server.cert_file, CERT_FILE, MAX_DIR_LEN);
	strncpy(server.key_file, KEY_FILE, MAX_DIR_LEN);
	server.client_list = init_list(&destroy_client);

	if (argc > 1)
		arg_parser(&server, argc, argv);

	daemonize_server();
	init_log();
	init_signal(&signal_exit);
	init_socket(&server);

	if (server.use_tls)
		init_tls(&server);

	init_epoll(&server);
	mhttp_listener(&server);

	for (;;)
		pause();

	exit(EXIT_SUCCESS);
}
