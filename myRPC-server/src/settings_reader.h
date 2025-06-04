#ifndef SETTINGS_READER_H
#define SETTINGS_READER_H

typedef struct {
    int port;
    char socket_type[16];
} Config;

Config parse_config(const char *filename);

#endif 
