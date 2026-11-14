#include <string.h>
#include <ctype.h>
#include "http.h"

static int find_header_end(const char *buffer,
        size_t buffer_size, size_t *header_end){
    
    if (buffer == NULL || header_end == NULL || buffer_size < 4) {
        return 0;
    }

    for (size_t i = 0; i <= buffer_size-4; i++) {
        if (buffer[i] == '\r' && buffer[i+1] == '\n' &&
            buffer[i+2] == '\r' && buffer[i+3] == '\n') {
                *header_end = i + 4;
                return 1;
        }
    }
    return 0;
}

static int is_valid_token(const char *buffer,
        size_t start,
        size_t end) {
    if (start >= end) {
        return 0;
    }

    for (size_t i = start; i < end; i++) {
        unsigned char c = (unsigned char)buffer[i];

        if (!isalnum(c) &&
            c != '!' && c != '#' && c != '$' &&
            c != '%' && c != '&' && c != '\'' &&
            c != '*' && c != '+' && c != '-' &&
            c != '.' && c != '^' && c != '_' &&
            c != '`' && c != '|' && c != '~') {
            return 0;
        }
    }
    return 1;
}

static int parse_request_line(const char *buffer, 
    size_t line_size, HTTPRequest *request) {
    size_t first_space = 0;
    size_t second_space = 0;
    
    if (buffer == NULL || request == NULL) {
    return 0;
    }

    while (first_space < line_size && buffer[first_space] != ' ') {
        first_space++;
    }

    if (first_space == 0 || first_space == line_size){
        return 0;
    }
    second_space = first_space + 1;

    while (second_space < line_size && buffer[second_space] != ' ') {
        second_space++;
    }

    if (second_space == first_space + 1 || second_space == line_size ||
        second_space + 1 >= line_size) {
            return 0;
    }

    for (size_t i = second_space + 1; i < line_size; i++) {
        if(buffer[i] == ' ') {
            return 0;
        }
    }

    size_t method_size = first_space;
    size_t uri_size = second_space - first_space - 1;
    size_t version_size = line_size -second_space -1;
    if (method_size >= sizeof(request->method) || 
        uri_size >= sizeof(request->uri) ||
        version_size >= sizeof(request->version) ||
        !is_valid_token(buffer, 0, first_space)) {
            return 0;
    }

    if (version_size != 8 ||
        memcmp(buffer + second_space + 1, "HTTP/1.1", 8) != 0) {
        return 0;
    }

    memcpy(request->method, buffer, method_size);
    request->method[method_size] = '\0';
    memcpy(request->uri, buffer + first_space + 1, uri_size);
    request->uri[uri_size] = '\0';
    memcpy(request->version, buffer + second_space +1, version_size);
    request->version[version_size] = '\0';

    return 1;
}

static int is_valid_header_name(const char *buffer,
                                size_t start,
                                size_t end) {
    if (start >= end) {
        return 0;
    }

    for (size_t i = start; i < end; i++) {
        unsigned char c = (unsigned char)buffer[i];

        if (!isalnum(c) &&
            c != '!' && c != '#' && c != '$' &&
            c != '%' && c != '&' && c != '\'' &&
            c != '*' && c != '+' && c != '-' &&
            c != '.' && c != '^' && c != '_' &&
            c != '`' && c != '|' && c != '~') {
            return 0;
        }
    }

    return 1;
}

static int parse_headers(const char *buffer,
    size_t start,
    size_t end,
    HTTPRequest *request) {
    size_t position = start;

    while (position < end) {
        size_t line_end = position;

        while (line_end + 1 < end &&
               !(buffer[line_end] == '\r' &&
                 buffer[line_end + 1] == '\n')) {
            line_end++;
        }

        if (line_end + 1 >= end) {
            return 0;
        }

        size_t colon = position;

        while (colon < line_end && buffer[colon] != ':') {
            colon++;
        }

        if (colon == position || colon == line_end) {
            return 0;
        }

        if (request->header_count >= HTTP_MAX_HEADERS) {
            return 0;
        }

        size_t name_size = colon - position;
        if (!is_valid_header_name(buffer, position, colon)) {
            return 0;
        }
        size_t value_start = colon + 1;

        while (value_start < line_end &&
               (buffer[value_start] == ' ' ||
                buffer[value_start] == '\t')) {
            value_start++;
        }

        size_t value_end = line_end;

        while (value_end > value_start &&
               (buffer[value_end - 1] == ' ' ||
                buffer[value_end - 1] == '\t')) {
            value_end--;
        }

        size_t value_size = value_end - value_start;

        if (name_size >= sizeof(request->headers[0].name) ||
            value_size >= sizeof(request->headers[0].value)) {
            return 0;
        }

        HTTPHeader *header = &request->headers[request->header_count];
        memcpy(header->name, buffer + position, name_size);
        header->name[name_size] = '\0';
        memcpy(header->value, buffer + value_start, value_size);
        header->value[value_size] = '\0';
        request->header_count++;
        position = line_end + 2;
    }

    return 1;
}

static int header_name_equals(const char *a, const char *b) {

    while (*a != '\0' && *b != '\0') {

        if (tolower((unsigned char)*a) !=
            tolower((unsigned char)*b)) {
            return 0;
        }

        a++;
        b++;
    }

    return *a == '\0' && *b == '\0';
}

static int validate_host_header(const HTTPRequest *request) {

    size_t host_count = 0;

    for (size_t i = 0; i < request->header_count; i++) {

        HTTPHeader *header = (HTTPHeader *)&request->headers[i];

        if (header_name_equals(header->name, "Host")) {

            host_count++;

            if (header->value[0] == '\0') {
                return 0;
            }
        }
    }

    return host_count == 1;
}

HTTPParseResult http_parse_request(const char *buffer,
    size_t buffer_size, HTTPRequest *request, size_t *consumed) {

    if (buffer == NULL || request == NULL || consumed == NULL) {
        return HTTP_PARSE_ERROR;
    }

    *consumed = 0;
    request->header_count = 0;
    size_t header_end = 0;

    if (!find_header_end(buffer, buffer_size, &header_end)) {
        return HTTP_PARSE_INCOMPLETE;
    }

    size_t line_end = 0;

    while (line_end + 1 < header_end && !(buffer[line_end] == '\r'
           && buffer[line_end + 1] == '\n')) {
        line_end++;    
    }

    if (line_end + 1 >= header_end || 
        !parse_request_line(buffer, line_end, request)) {
        return HTTP_PARSE_ERROR;
    }

    size_t headers_start = line_end + 2;
    size_t headers_end = header_end - 2;

    if (!parse_headers(buffer, headers_start, headers_end, request )) {
        return HTTP_PARSE_ERROR;
    }

    if (!validate_host_header(request)) {
        return HTTP_PARSE_ERROR;
    }

    *consumed = header_end;
    return HTTP_PARSE_COMPLETE;
}