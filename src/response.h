#ifndef RESPONSE_H
#define RESPONSE_H

#include "request.h"

typedef enum {
    CT_TEXT_PLAIN = 0,
    CT_OCTET_STREAM,
    CT_TEXT_HTML,
    CT_STYLE_CSS,
    CT_APPLICATION_JAVASCRIPT,
    CT_APPLICATION_JSON,
    CT_IMAGE_PNG,
    CT_IMAGE_JPEG,
    CT_IMAGE_GIF,
    CT_IMAGE_SVG,
    CT_APPLICATION_PDF,
    /* guard */
    CT_COUNT
} content_type_e;
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
int serialize_response(struct response *, struct request *, char *, int);
int send_response(int, struct response *, struct request *, int);
int stream_fd_to_client(int, int);
#endif