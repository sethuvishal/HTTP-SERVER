#ifndef RESPONSE_H
#define RESPONSE_H

#include "request.h"

typedef enum {
    R_HTTP_OK = 0,
    R_NOT_FOUND,
    /* guard */
    R_COUNT
} resp_status_e;

struct response {
    resp_status_e status;
    content_type_e type;
    int has_content_length;
    long content_length;
    char *content;
};
char *server_status_responses[R_COUNT] = {
    [R_HTTP_OK] = "HTTP/1.1 200 OK",
    [R_NOT_FOUND] = "HTTP/1.1 404 Not Found",
};
char *server_content_types[CT_COUNT] = {
    [CT_TEXT_PLAIN] = "text/plain",
    [CT_OCTET_STREAM] = "application/octet-stream",
};

void free_resp(struct response *);
int send_all(int, const char *, size_t);
int serialize_response(struct response *, char *, int);
int send_response(int, struct response *, int);
int stream_fd_to_client(int, int);
#endif