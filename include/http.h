#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

#define HTTP_MAX_HEADERS 50

typedef struct{
    char name[128];
    char value[1024];
}HTTPHeader;

typedef struct{
    char method[16];
    char uri[2048];
    char version[16];

    HTTPHeader headers[HTTP_MAX_HEADERS];
    size_t header_count;
}HTTPRequest;

typedef enum {
    HTTP_PARSE_INCOMPLETE = 0,
    HTTP_PARSE_COMPLETE = 1,
    HTTP_PARSE_ERROR = -1
} HTTPParseResult;

HTTPParseResult http_parse_request(
    const char *buffer,
    size_t buffer_size,
    HTTPRequest *request,
    size_t *consumed
);

#endif