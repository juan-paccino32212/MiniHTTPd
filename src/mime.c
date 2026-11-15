#include <string.h>
#include "mime.h"

const char *get_mime_type(const char *path) {
    if (path == NULL) {
        return "application/octet-stream";
    }
    const char *extension = strrchr(path, '.');

    if (extension == NULL) {
       return "application/octet-stream";
    }

    if (strcmp(extension, ".html") == 0 ||
        strcmp(extension, ".htm") == 0) {
        return "text/html";
    }

    if (strcmp(extension, ".css") == 0) {
        return "text/css";
    } else if (strcmp(extension, ".js") == 0) {
        return "application/javascript";
    } else if (strcmp(extension, ".json") == 0) {
        return "application/json";
    } else if (strcmp(extension, ".png") == 0) {
        return "image/png";
    } else if (strcmp(extension, ".jpg") == 0) {
        return "image/jpeg";
    } else if (strcmp(extension, ".jpeg") == 0) {
        return "image/jpeg";
    } else if (strcmp(extension, ".gif") == 0) {
        return "image/gif";
    } else if (strcmp(extension, ".svg") == 0) {
        return "image/svg+xml";
    } else if (strcmp(extension, ".txt") == 0) {
        return "text/plain";
    } else if (strcmp(extension, ".pdf") == 0) {
        return "application/pdf";
    } else {
    return "application/octet-stream";
    }
}
