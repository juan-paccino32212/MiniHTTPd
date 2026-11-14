#ifndef FILES_H
#define FILES_H

#include <stddef.h>

int read_file(const char *path,
              char **content,
              size_t *content_size);

#endif