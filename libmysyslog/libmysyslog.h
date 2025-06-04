#ifndef LIBMYSYSLOG_H
#define LIBMYSYSLOG_H
#define DEBUG 0
#define INFO 1
#define WARN 2
#define ERROR 3
#define CRITICAL 4
int mysyslog(const char* msg_content, int severity_level, int component_id, int output_style, const char* log_filepath);
#endif // LIBMYSYSLOG_H
