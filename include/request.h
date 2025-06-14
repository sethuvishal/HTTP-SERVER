#ifndef REQUEST_H
#define REQUEST_H
#include <glib.h>

struct request {
    char* method;
    int content_length;
    char *path;
    char *host;
    char *agent;
    char *http_version;
    char *content;
    char *connection;
    char *cookies;
    GHashTable *headers;
};

struct connection {
    int fd;
    char *buffer;
    int buffer_size;
    int received;
    struct request *req;
};

void print_request(struct request*);

void free_req(struct request*);

void free_conn(struct connection*);

int recieve_request(int, struct connection*);

int parse_request(char *, size_t , struct request *);

int try_parse_field(char *, const char *, char **, int );
#endif