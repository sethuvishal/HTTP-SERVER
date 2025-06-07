#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

void handle_request(struct request* req, int client_fd){
  struct response *resp = malloc(sizeof(struct response));
  memset(resp, 0, sizeof(struct response));
  if (!resp) {
    fprintf(stderr, "Memory allocation failed\n");
    free_req(req);
    return;
  }

  if (strcmp(req->path, "/") == 0) {
    resp->status = R_HTTP_OK;
    resp->content = strdup("Example Response");
    resp->type = CT_TEXT_PLAIN;
    send_response(client_fd, resp, req, NULL);
  } else if (strstr(req->path, "echo")) {
    resp = echo_handler(req);
    send_response(client_fd, resp, req, NULL);
  } else if (strcmp(req->path, "/user-agent") == 0) {
    resp->status = R_HTTP_OK;
    resp->content = strdup(req->agent);
    resp->type = CT_TEXT_PLAIN;
    send_response(client_fd, resp, req, NULL);
  } else if (strncmp(req->path, "/files/", 7) == 0) {
    resp = serve_file(req);
    if(resp->status == R_HTTP_OK){
      int fd = get_file_fd(&req->path[strlen("/files/")]);
      send_response(client_fd, resp, req, fd == -1 ? 0 : fd);
      close(fd);
    }else{
      send_response(client_fd, resp, req, 0);
    }
  }else {
    resp->status = R_NOT_FOUND;
    send_response(client_fd, resp, req, NULL);
  }

  if(strcmp(req->connection, "keep-alive") != 0){
    free_req(req);
    free_resp(resp);
    return;
  }

  free_req(req);
  free_resp(resp);
}

int main(){

}