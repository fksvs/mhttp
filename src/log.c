#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <unistd.h>
#include "log.h"

static console_log console = { .stream = 2,
			       .log_level = LOG_TRACE,
			       .quiet = false,
			       .color = false,
			       .log_format = DEFAULT_FORMAT };

static file_log file_logs[MAX_FILE_LOG];
static int file_log_ind = 0;

static const char *level_string[] = { "FATAL",	"CRITICAL", "ERROR", "WARNING",
				      "NOTICE", "INFO",	    "DEBUG", "TRACE" };
static const char *level_color[] = { RED,    RED,   RED,  RED,
				     YELLOW, GREEN, CYAN, BLUE };

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void lock()
{
	pthread_mutex_lock(&mutex);
}

static void unlock()
{
	pthread_mutex_unlock(&mutex);
}

void set_stream(int stream)
{
	if (stream == 1 || stream == 2)
		console.stream = stream;
	else
		fprintf(stderr,
			"stream should be one of the stdout or stderr\n");
}

void set_level(int log_level)
{
	if (log_level < LOG_FATAL)
		log_level = LOG_FATAL;
	else if (log_level > LOG_TRACE)
		log_level = LOG_TRACE;

	console.log_level = log_level;
}

void set_quiet(bool quiet)
{
	console.quiet = quiet;
}

void set_color(bool color)
{
	console.color = color;
}

void set_format(char *log_format)
{
	console.log_format = log_format;
}

void init_console(int stream, int log_level, bool quiet, bool color,
		  char *log_format)
{
	set_stream(stream);
	set_level(log_level);
	set_quiet(quiet);
	set_color(color);
	set_format(log_format);
}

int init_file_log(char *filename, int log_level, int log_type,
		  size_t max_file_size, size_t max_rotation)
{
	int fd;

	lock();

	if ((fd = open(filename, O_APPEND | O_WRONLY | O_CREAT,
		       S_IRUSR | S_IWUSR)) == -1) {
		fprintf(stderr, "open : %s\n", strerror(errno));
		return -1;
	}

	memset(&file_logs[file_log_ind], 0, sizeof(file_log));
	strncpy(file_logs[file_log_ind].filename, filename, strlen(filename));
	file_logs[file_log_ind].fd = fd;
	file_logs[file_log_ind].log_level = log_level;
	file_logs[file_log_ind].log_type = log_type;
	file_logs[file_log_ind].max_file_size = max_file_size;
	file_logs[file_log_ind].total_file = 1;
	file_logs[file_log_ind].max_rotation = max_rotation;
	file_log_ind += 1;

	unlock();

	return fd;
}

int init_basic_log(char *filename, int log_level)
{
	return init_file_log(filename, log_level, BASIC_LOG, 0, 0);
}

int init_rotate_log(char *filename, int log_level, size_t max_file_size,
		    size_t max_rotation)
{
	return init_file_log(filename, log_level, ROTATE_LOG, max_file_size,
			     max_rotation);
}

int add_fd_log(int fd, int log_level)
{
	lock();

	memset(&file_logs[file_log_ind], 0, sizeof(file_log));
	file_logs[file_log_ind].fd = fd;
	file_logs[file_log_ind].log_level = log_level;
	file_logs[file_log_ind].log_type = BASIC_LOG;
	file_log_ind += 1;

	unlock();

	return fd;
}

void close_log_files()
{
	int ind;
	
	lock();
	for (ind = 0; ind < file_log_ind; ind += 1)
		close(file_logs[ind].fd);
	unlock();
}

static int rotate_file(file_log *file)
{
	int fd;
	char new_filename[PATH_SIZE + FORMAT_SIZE];
	char timestamp[FORMAT_SIZE], *ptr;
	time_t t = time(NULL);
	struct tm *ts = localtime(&t);
	strftime(timestamp, FORMAT_SIZE, "%Y%m%d_%H%M%S", ts);

	ptr = strchr(file->filename, '.');
	if (!ptr) {
		sprintf(new_filename, "%s%s", file->filename, timestamp);
	} else {
		char temp[strlen(file->filename)];
		strcpy(temp, file->filename);
		temp[strlen(temp) - strlen(ptr)] = '\0';
		sprintf(new_filename, "%s%s%s", temp, timestamp, ptr);
	}

	if ((fd = open(new_filename, O_APPEND | O_WRONLY | O_CREAT,
		       S_IRUSR | S_IWUSR)) == -1) {
		fprintf(stderr, "open : %s\n", strerror(errno));
		return -1;
	}

	close(file->fd);
	file->fd = fd;
	file->total_file += 1;

	return fd;
}

static int check_rotate(file_log *file, size_t msg_size)
{
	struct stat st;

	if (fstat(file->fd, &st) == -1) {
		fprintf(stderr, "fstat : %s\n", strerror(errno));
		return -1;
	}

	if (st.st_size >= file->max_file_size) {
		if (file->total_file < file->max_rotation)
			return rotate_file(file);
		return -1;
	} else if ((file->max_file_size - st.st_size) < msg_size) {
		if (file->total_file < file->max_rotation)
			return rotate_file(file);
		return -1;
	} else {
		return 0;
	}
}

static void create_log_msg(char *buffer, log_msg *msg)
{
	char *c, *ptr = buffer;
	struct tm *t = localtime(&msg->log_time);

	for (c = console.log_format; *c != '\0'; c += 1) {
		if (*c == '%') {
			c += 1;
			switch (*c) {
			case 'T':
				ptr += strftime(ptr, FORMAT_SIZE,
						"%d.%m.%Y %H:%M:%S", t);
				break;
			case 'd':
				ptr += sprintf(ptr, "%02d", t->tm_mday);
				break;
			case 'm':
				ptr += sprintf(ptr, "%02d", t->tm_mon + 1);
				break;
			case 'Y':
				ptr += sprintf(ptr, "%d", t->tm_year + 1900);
				break;
			case 'H':
				ptr += sprintf(ptr, "%02d", t->tm_hour);
				break;
			case 'M':
				ptr += sprintf(ptr, "%02d", t->tm_min);
				break;
			case 'S':
				ptr += sprintf(ptr, "%02d", t->tm_sec);
				break;
			case 'L':
				ptr += sprintf(ptr, "%s", level_string[msg->log_level]);
				break;
			case 'P':
				ptr += sprintf(ptr, "%d", msg->log_pid);
				break;
			case 't':
				ptr += sprintf(ptr, "%d", msg->log_tid);
				break;
			case 'U':
				ptr += sprintf(ptr, "%d", msg->log_uid);
				break;
			case 'l':
				ptr += sprintf(ptr, "%d", msg->log_line);
				break;
			case 'F':
				ptr += sprintf(ptr, "%s", msg->log_file);
				break;
			case 'A':
				ptr += vsprintf(ptr, msg->msg_format, msg->msg_ap);
				break;
			case '%':
				*ptr = *c;
				ptr += 1;
				break;
			}
		} else {
			*ptr = *c;
			ptr += 1;
		}
	}
}

static void write_console(const char *data, int log_level)
{
	if (console.log_level >= log_level && console.quiet == false) {
		if (console.color == true)
			dprintf(console.stream, "%s%s%s",
				level_color[log_level], data, COLOR_RESET);
		else
			write(console.stream, data, strlen(data));
	}
}

static void write_log_file(const char *data, int log_level)
{
	int ind;

	for (ind = 0; ind < file_log_ind; ind += 1) {
		if (file_logs[ind].log_type == ROTATE_LOG) {
			if (check_rotate(&file_logs[ind], strlen(data)) == -1)
				continue;
		}

		if (file_logs[ind].log_level >= log_level)
			write(file_logs[ind].fd, data, strlen(data));
	}
}

void cloglib_log(int log_level, const char *log_file, int log_line,
		 const char *msg_format, ...)
{
	log_msg msg;
	char data[MSG_SIZE];

	msg.log_time = time(NULL);
	msg.log_level = log_level;
	msg.log_pid = getpid();
	msg.log_tid = syscall(SYS_gettid);
	msg.log_uid = getuid();
	msg.log_line = log_line;
	msg.log_file = log_file;
	msg.msg_format = msg_format;
	va_start(msg.msg_ap, msg_format);

	memset(data, 0, MSG_SIZE);
	create_log_msg(data, &msg);
	
	lock();
	write_console(data, log_level);
	write_log_file(data, log_level);
	unlock();

	va_end(msg.msg_ap);
}
