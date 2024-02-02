#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include "types.h"

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
}

void mhttp_usage()
{
	fprintf(stdout, "\n usage: ./mhttp [options]\n\n options:\n\
\t-a [listen address] : listen address for incoming connection\n\
\t-p [listen port] : listen port for incoming connections\n\
\t-d [directory] : main directory to serve\n\
\t-s : use TLS\n\
\t-c [certificate file] : TLS certificate file\n\
\t-k [key file] : TLS private key file\n\
\t-h : this help message\n\n");
}

void arg_parser(struct server_t *server, int argc, char *argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "a:p:d:sc:k:h")) != -1) {
		switch (opt) {
		case 'a':
			strncpy(server->listen_address, optarg,
				INET_ADDRSTRLEN);
			break;
		case 'p':
			server->listen_port = atoi(optarg);
			break;
		case 'd':
			strncpy(server->working_dir, optarg, MAX_DIR_LEN);
			break;
		case 's':
			server->use_tls = true;
			break;
		case 'c':
			strncpy(server->cert_file, optarg, MAX_DIR_LEN);
			break;
		case 'k':
			strncpy(server->key_file, optarg, MAX_DIR_LEN);
			break;
		case 'h':
			mhttp_usage();
			exit(EXIT_SUCCESS);
		}
	}
}
