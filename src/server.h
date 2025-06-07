#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "request.h"
#include "response.h"

void start_server(int port, struct response* (*handle_request)(struct request*, int fd));

#endif
