#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "settings_reader.h"

Settings read_settings(const char *filepath) {
    Settings settings;
    settings.port_number = 0;
    strcpy(settings.socket_kind, "stream");

    FILE *config_file = fopen(filepath, "r");
    if (!config_file) {
        perror("Unable to open settings file");
        return settings;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), config_file)) {
        // Удаление символа новой строки
        buffer[strcspn(buffer, "\n")] = '\0';

        // Пропуск комментариев и пустых строк
        if (buffer[0] == '#' || strlen(buffer) == 0)
            continue;

        char *key_part = strtok(buffer, "=");
        char *value_part = strtok(NULL, "");

        if (strcmp(key_part, "port") == 0) {
            settings.port_number = atoi(value_part);
        } else if (strcmp(key_part, "socket_type") == 0) {
            strcpy(settings.socket_kind, value_part);
        }
    }

    fclose(config_file);
    return settings;
}
