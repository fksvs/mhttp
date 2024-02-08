#ifndef MHTTP_LOG_H
#define MHTTP_LOG_H

#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/types.h>

#define MSG_SIZE 10000
#define MAX_FILE_LOG 100
#define PATH_SIZE 256
#define FORMAT_SIZE 100

typedef struct {
	int stream;
	int log_level;
	int quiet;
	int color;
	char *log_format;
} console_log;

#define BASIC_LOG 0
#define ROTATE_LOG 1

typedef struct {
	char filename[PATH_SIZE];
	int fd;
	int log_level;
	int log_type;
	size_t max_file_size;
	size_t total_file;
	size_t max_rotation;
} file_log;

typedef struct {
	time_t log_time;
	int log_level;
	pid_t log_pid;
	pid_t log_tid;
	uid_t log_uid;
	int log_line;
	const char *log_file;
	const char *msg_format;
	va_list msg_ap;
} log_msg;

#define DEFAULT_FORMAT "[%T] %L [%F:%l] %A\n"

enum {
	LOG_FATAL,
	LOG_CRIT,
	LOG_ERROR,
	LOG_WARN,
	LOG_NOTICE,
	LOG_INFO,
	LOG_DEBUG,
	LOG_TRACE
};

#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define CYAN "\033[0;36m"
#define COLOR_RESET "\033[0m"

void set_stream(int stream);
void set_level(int log_level);
void set_quiet(bool quiet);
void set_color(bool color);
void set_format(char *log_format);
void init_console(int stream, int log_level, bool quiet, bool color,
		  char *log_format);

int init_file_log(char *filename, int log_level, int log_type,
		  size_t max_file_size, size_t max_rotation);
int init_basic_log(char *filename, int log_level);
int init_rotate_log(char *filename, int log_level, size_t max_file_size,
		    size_t max_rotation);
int add_fd_log(int fd, int log_level);

void close_log_files();

#define log_fatal(...) cloglib_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)
#define log_critical(...) cloglib_log(LOG_CRIT, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) cloglib_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_warning(...) cloglib_log(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define log_notice(...) cloglib_log(LOG_NOTICE, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...) cloglib_log(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) cloglib_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_trace(...) cloglib_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)

void cloglib_log(int log_level, const char *log_file, int log_line,
		 const char *msg_format, ...);

#endif
