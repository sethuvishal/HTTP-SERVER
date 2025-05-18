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
extern const char *server_status_responses[R_COUNT];
extern const char *server_content_types[CT_COUNT];

void free_resp(struct response *);
int send_all(int, const char *, size_t);
int serialize_response(struct response *, char *, int);
int send_response(int, struct response *, int);
int stream_fd_to_client(int, int);
#endif