#ifndef FILES_H
#define FILES_H

#include <stddef.h>

int read_file(const char *path, char **content,
                          size_t *content_size);

typedef enum {
    PATH_OK,
    PATH_NOT_FOUND,
    PATH_FORBIDDEN,
    PATH_INTERNAL_ERROR
} PathResult;

PathResult resolve_path(const char *uri, char *resolved_path,
                        size_t path_size);

#endif