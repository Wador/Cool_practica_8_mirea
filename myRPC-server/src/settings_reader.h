#ifndef SETTINGS_READER_H
#define SETTINGS_READER_H

typedef struct {
    int port_number;
    char socket_kind[256];
} Settings;

Settings read_settings(const char *filepath);

#endif // SETTINGS_READER_H
