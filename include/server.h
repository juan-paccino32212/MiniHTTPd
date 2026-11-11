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
    size_t read_capacity;

    int closing;

    struct Client *next;
} Client;

int server_start(void);

#endif