#ifndef SERVER_H
#define SERVER_H

#include <stddef.h>

typedef enum {
    CLIENT_READING,
    CLIENT_WRITING,
    CLIENT_CLOSING
} ClientState;

typedef struct Client {
    int fd;
    ClientState state;

    char *read_buffer;
    size_t read_size;
    size_t request_consumed;
    size_t read_capacity;
    int read_eof;
    char *write_buffer;
    size_t write_size;
    size_t write_sent;

    int closing;
    int keep_alive;

    struct Client *next;
} Client;

int server_start(void);

#endif