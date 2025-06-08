#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "request.h"
#include "response.h"

int start_server(int port, void (*handle_request)(struct request*, struct response*, int fd));

#endif
