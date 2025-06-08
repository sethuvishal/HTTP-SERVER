#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "server.h"

void handle_request(struct request* req, struct response* resp, int client_fd){
  if (strcmp(req->path, "/") == 0) {
    resp->status = R_HTTP_OK;
    resp->content = strdup("Example Response");
    resp->type = CT_TEXT_PLAIN;
    send_response(client_fd, resp, req, 0);
  } else if (strcmp(req->path, "/user-agent") == 0) {
    resp->status = R_HTTP_OK;
    resp->content = strdup(req->agent);
    resp->type = CT_TEXT_PLAIN;
    send_response(client_fd, resp, req, 0);
  } else if (strncmp(req->path, "/files/", 7) == 0) {
    char* filename = strdup(req->path + strlen("/files/"));
    serve_file(client_fd, req, resp, filename);
  }else if(strncmp(req->path, "/favicon", 8) == 0){
    char* filename = strdup("./favicon.png");
    serve_file(client_fd, req, resp, filename);
  }else {
    resp->status = R_NOT_FOUND;
    resp->content = strdup("Not Found");
    resp->content_length = strlen(resp->content);
    send_response(client_fd, resp, req, 0);
  }

  if(strcmp(req->connection, "keep-alive") != 0){
    return;
  }
}

int main(){
  start_server(8000, handle_request);
}