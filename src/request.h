#ifndef REQUEST_H
#define REQUEST_H

typedef enum {
    CT_TEXT_PLAIN = 0,
    CT_OCTET_STREAM,
    CT_TEXT_HTML,
    /* guard */
    CT_COUNT
} content_type_e;

struct request {
    char* method;
    int content_length;
    char *path;
    char *host;
    char *agent;
    char *http_version;
    char *content;
};

void print_request(struct request*);

void free_req(struct request*);

int recieve_request(int, struct request*);

int parse_request(char *, size_t , struct request *);

int try_parse_field(char *, char *, char **, int );
#endif