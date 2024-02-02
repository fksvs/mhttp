#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include "types.h"
#include "utils.h"
#include "init.h"
#include "request.h"
#include "response.h"
#include "server.h"

int main(int argc, char *argv[])
{
	struct server_t server;

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
                arg_parser(&server, argc, argv);

        // daemonize_server(&server);
        init_log();
	init_signal();
        init_socket(&server);
        init_tls(&server);
        init_epoll(&server);
        mhttp_listener(&server);

        for (;;)
                pause();

        exit(EXIT_SUCCESS);
}
