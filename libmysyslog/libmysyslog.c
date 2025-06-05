#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libmysyslog.h"

int mysyslog(const char* msg_content, int severity_level, int component_id, int output_style, const char* log_filepath) {
    FILE *log_stream = fopen(log_filepath, "a");
    if (log_stream == NULL) {
        return -1;
    }

    time_t current_time;
    time(&current_time);
    char *formatted_time = ctime(&current_time);
    formatted_time[strlen(formatted_time) - 1] = '\0';  // Удаление символа новой строки

    const char *severity_text;
    switch (severity_level) {
        case DEBUG:    severity_text = "DEBUG"; break;
        case INFO:     severity_text = "INFO"; break;
        case WARN:     severity_text = "WARNING"; break;
        case ERROR:    severity_text = "ERROR"; break;
        case CRITICAL: severity_text = "CRITICAL"; break;
        default:       severity_text = "UNDEFINED"; break;
    }

    if (output_style == 0) {
        // Текстовый формат
        fprintf(log_stream, "%s [%s] (ID: %d) %s\n", formatted_time, severity_text, component_id, msg_content);
    } else {
        // JSON-формат
        fprintf(log_stream, "{\"time\":\"%s\",\"level\":\"%s\",\"id\":%d,\"message\":\"%s\"}\n",
                formatted_time, severity_text, component_id, msg_content);
    }

    fclose(log_stream);
    return 0;
}
