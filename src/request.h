#ifndef REQUEST_H
#define REQUEST_H

struct request {
    char* method;
    int content_length;
    char *path;
    char *host;
    char *agent;
    char *http_version;
    char *content;
    char *connection;
};

void print_request(struct request*);

void free_req(struct request*);

int recieve_request(int, struct request*);

int parse_request(char *, size_t , struct request *);

int try_parse_field(char *, char *, char **, int );
#endif