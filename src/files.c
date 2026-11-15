#define _XOPEN_SOURCE 700

#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "files.h"

int read_file(const char *path, char **content,
                          size_t *content_size) {
    if (path == NULL || content == NULL ||
        content_size == NULL) {
        return -1;
    }

    *content = NULL;
    *content_size = 0;
    FILE *file = fopen(path, "rb");

    if (file == NULL) {
        perror("Error al abrir el archivo");
        return -1;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return -1;
    }

    long file_size = ftell(file);

    if (file_size < 0) {
        fclose(file);
        return -1;
    }

    if ((uintmax_t)file_size > SIZE_MAX) {
        fclose(file);
        return -1;
    }

    size_t allocation_size = (size_t)file_size;

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }

    char *buffer = malloc(allocation_size > 0
                          ? allocation_size : 1);

    if (buffer == NULL) {
        fclose(file);
        return -1;
    }

    size_t bytes_read = fread(buffer, 1, allocation_size, file);

    if (bytes_read != allocation_size || ferror(file)) {
        free(buffer);
        fclose(file);
        return -1;
    }

    if (fclose(file) != 0) {
        free(buffer);
        return -1;
    }

    *content = buffer;
    *content_size = bytes_read;

    return 0;
}

static int get_web_root(char *root, size_t root_size) {
    char *resolved = realpath("www", NULL);

    if (resolved == NULL) {
        return -1;
    }

    size_t length = strlen(resolved);

    if (length + 1 > root_size) {
        free(resolved);
        return -1;
    }

    memcpy(root, resolved, length + 1);
    free(resolved);

    return 0;
}

static PathResult classify_path_error(int error) {
    switch (error) {
    case ENOENT:
    case ENOTDIR:
        return PATH_NOT_FOUND;
    case EACCES:
    case ELOOP:
        return PATH_FORBIDDEN;
    case EPERM:
        return PATH_FORBIDDEN;
    default:
        return PATH_INTERNAL_ERROR;
    }
}

PathResult resolve_path(const char *uri, char *resolved_path,
                                     size_t path_size) {
    if (resolved_path == NULL || path_size == 0) {
        return PATH_INTERNAL_ERROR;
    }

    if (uri == NULL || uri[0] != '/') {
        return PATH_FORBIDDEN;
    }

    /* Rechazar caracteres que no admitiremos en esta versión */
    if (strchr(uri, '%') != NULL ||
        strchr(uri, '\\') != NULL) {
        return PATH_FORBIDDEN;
    }

    char root[PATH_MAX];

    if (get_web_root(root, sizeof(root)) == -1) {
        return PATH_INTERNAL_ERROR;
    }

    /* Separar la ruta de la query string */
    size_t uri_length = strcspn(uri, "?#");

    if (uri_length == 0) {
        return PATH_FORBIDDEN;
    }

    char relative_path[PATH_MAX];

    if (uri_length == 1 && uri[0] == '/') {
        int path_length = snprintf(relative_path,
                                   sizeof(relative_path),
                                   "%s/index.html", root);

        if (path_length < 0 ||
            (size_t)path_length >= sizeof(relative_path)) {
            return PATH_INTERNAL_ERROR;
        }
    } else {
        int path_length = snprintf(relative_path,
                                   sizeof(relative_path),
                                   "%s%.*s", root,
                                   (int)uri_length, uri);

        if (path_length < 0 ||
            (size_t)path_length >= sizeof(relative_path)) {
            return PATH_INTERNAL_ERROR;
        }
    }

    char *resolved = realpath(relative_path, NULL);

    if (resolved == NULL) {
        return classify_path_error(errno);
    }

    size_t root_length = strlen(root);
    size_t resolved_length = strlen(resolved);

    /* Comprobar que la ruta resuelta esté dentro de www */
    if (resolved_length < root_length ||
        strncmp(resolved, root, root_length) != 0 ||
        (resolved_length > root_length &&
         resolved[root_length] != '/')) {
        free(resolved);
        return PATH_FORBIDDEN;
    }

    struct stat file_info;

    if (stat(resolved, &file_info) == -1) {
        PathResult result = classify_path_error(errno);
        free(resolved);
        return result;
    }

    if (!S_ISREG(file_info.st_mode)) {
        free(resolved);
        return PATH_FORBIDDEN;
    }

    if (resolved_length + 1 > path_size) {
        free(resolved);
        return PATH_INTERNAL_ERROR;
    }

    memcpy(resolved_path, resolved, resolved_length + 1);
    free(resolved);

    return PATH_OK;
}