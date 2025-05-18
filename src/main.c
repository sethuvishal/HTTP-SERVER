#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "request.h"
#include "response.h"
#include "config.h"
#include "utils.h"

struct response* echo_handler(struct request* req){
  char *random_string = NULL, *echo = NULL;
  struct response* resp = malloc(sizeof(struct response));
  if (try_parse_field(req->path, slash_char, &echo, 1) ||
    try_parse_field(req->path, slash_char, &random_string, 2) ||
    strcmp(echo, "echo") != 0) {
  resp->status == R_NOT_FOUND;
  } else {
  resp->status = R_HTTP_OK;
  resp->content = random_string;
  resp->type = CT_TEXT_PLAIN;
  }
  if (echo) free(echo);
  if (resp->status != R_HTTP_OK && random_string)
    free(random_string);
  return resp;
}

struct response* serve_file(struct request* req){
  char *prefix = "/files/";
  char *filename = &req->path[strlen(prefix)];
  struct response* resp = malloc(sizeof(struct response));

  if(filename == NULL){
    resp->status = R_NOT_FOUND;
    resp->content = prefix;
    resp->type = CT_TEXT_PLAIN;
    return resp;
  }
  char cwd[PATH_MAX];
  char fullpath[PATH_MAX];

  if(getcwd(cwd, sizeof(cwd)) == NULL){
    resp->status = R_NOT_FOUND;
    resp->content = strdup("Something went wrong!");
    resp->type = CT_TEXT_PLAIN;
    return resp;
  }
  snprintf(fullpath, sizeof(fullpath), "%s/%s", cwd, filename);
  // Check if file exists
  if (access(fullpath, F_OK) != 0) {
      resp->status = R_NOT_FOUND;
      resp->content = strdup("File not found");
      resp->type = CT_TEXT_PLAIN;
      return resp;
  }

  FILE *fp = fopen(fullpath, "r");
  fseek(fp, 0, SEEK_END);
  long filesize = ftell(fp);
  fclose(fp);
  resp->status = R_HTTP_OK;
  const char* file_ext = get_file_extension(filename);
  content_type_e content_type = get_content_type(file_ext);
  resp->type = content_type;
  resp->has_content_length = 1;
  resp->content_length = filesize;
  resp->content = NULL;

  return resp;
}

void handle_connection(int client_fd){
  while(1){
    struct request *req = malloc(sizeof(struct request));
    struct response *resp = malloc(sizeof(struct response));
    if (!req || !resp) {
      fprintf(stderr, "Memory allocation failed\n");
      exit(1);
    }
    memset(resp, 0, sizeof(struct response));
    if (recieve_request(client_fd, req)) {
      free_req(req);
      free_resp(resp);
      break;  // EXIT the loop on error
    }
    printf("Request Path: %s\n", req->path);
    /* respond to the request */
    memset(resp, 0, sizeof(struct response));
    if (strcmp(req->path, "/") == 0) {
      resp->status = R_HTTP_OK;
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
      break;
    }
    free_req(req);
    free_resp(resp);
  }
  printf("closing the connection %d\n", getpid());
  close(client_fd);
  exit(0);
}

int main() {
  // Disable output buffering
  setbuf(stdout, NULL);
  // You can use print statements as follows for debugging, they'll be visible
  // when running tests.
  ssize_t recv_len;
  int server_fd, client_addr_len, client_fd;
  struct sockaddr_in client_addr;
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    return 1;
  } 
  // setting REUSE_PORT ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) <
      0) {
    return 1;
  }
  struct sockaddr_in serv_addr = {
      .sin_family = AF_INET,
      .sin_port = htons(4221),
      .sin_addr = {htonl(INADDR_ANY)},
  };
  /* bind created socket to chosen addr:port combo */
  if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
    printf("Bind failed: %s \n", strerror(errno));
    return 1;
  }
  /* listen for clients */
  int connection_backlog = 5;
  printf("Listening on port %d...\n", ntohs(serv_addr.sin_port));
  if (listen(server_fd, connection_backlog) != 0) {
    printf("Listen failed: %s \n", strerror(errno));
    return 1;
  }
  printf("Waiting for a client to connect...\n");
  while(1){
    client_addr_len = sizeof(client_addr);
    /* if any client desires to connect -- accept */
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    size_t main_pid = getpid();
    size_t pid = fork();
    if (pid != 0) {
      printf("Pid %d\n", pid);
      close(client_fd);
      continue;
    }
    if (client_fd == -1) {
      printf("Failed to connect to client: %s\n", strerror(errno));
    }
    /* parse client request */
    printf("Client Connected %d\n", client_fd);
    handle_connection(client_fd);
  }
  close(server_fd);
  return 0;
}