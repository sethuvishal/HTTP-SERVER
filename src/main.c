#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define RECV_BUFFER_SIZE 1024
typedef enum {
  R_HTTP_OK = 0,
  R_NOT_FOUND,
  /* guard */
  R_COUNT
} resp_status_e;
typedef enum {
  CT_TEXT_PLAIN = 0,
  /* guard */
  CT_COUNT
} content_type_e;
struct request {
  char *path;
  char *host;
  char *agent;
  char *http_version;
  char *content;
};
struct response {
  resp_status_e status;
  content_type_e type;
  char *content;
};
char *server_status_responses[R_COUNT] = {
    [R_HTTP_OK] = "HTTP/1.1 200 OK",
    [R_NOT_FOUND] = "HTTP/1.1 404 Not Found ",
};
char *server_content_types[R_COUNT] = {
    [CT_TEXT_PLAIN] = "text/plain",
};
char *newline_char = "\r\n";
char *space_char = " ";
char *slash_char = "/";
void free_req(struct request *req) {
  if (req->path)
    free(req->path);
  if (req->host)
    free(req->host);
  if (req->agent)
    free(req->agent);
  if (req->http_version)
    free(req->http_version);
  if (req->content)
    free(req->content);
  free(req);
}
void free_resp(struct response *resp) {
  if (resp->content)
    free(resp->content);
  free(resp);
}
void print_request(struct request *req) {
  printf("Request object:\n");
  printf("\t path: %s\n", req->path);
  printf("\t version: %s\n", req->http_version);
  printf("\t host: %s\n", req->host);
  printf("\t agent: %s\n", req->agent);
  printf("\t content: %s\n", req->content);
}
int send_response(int cfd, char *resp) {
  ssize_t resp_len, sent_bytes, remaining;
  resp_len = strlen(resp);
  remaining = resp_len;
  while (remaining) {
    sent_bytes = send(cfd, &resp[resp_len - remaining], remaining, 0);
    if (sent_bytes < 0) {
      printf("Failed to send response to client : %s\n", strerror(errno));
      break;
    }
    remaining -= sent_bytes;
  }
  return (remaining != 0);
}
int try_parse_field(char *buf, char *delim, char **out, int fnum) {
  int token_count = 0, rc = 1;
  char *field;
  char *buf_copy = strdup(buf);
  if (!buf_copy) {
    return -1;
  }
  char *token = strtok(buf_copy, delim);
  while (token && (token_count < fnum)) {
    field = token;
    token_count++;
    token = strtok(NULL, delim);
  }
  if (token_count == fnum) {
    *out = strdup(field);
    rc = 0;
  }
  free(buf_copy);
  return rc;
}
int parse_request(char *buf, struct request *req) {
  char *save_line, *line, *field;
  int cntr;
  int rc = 1;
  memset(req, 0, sizeof(struct request));
  /* parse path */
  line = strtok_r(buf, newline_char, &save_line);
  if (!line || try_parse_field(line, space_char, &req->path, 2)) {
    printf("Failed to parse request path");
    goto exit;
  }
  /* parse http version */
  if (!line || try_parse_field(line, space_char, &req->http_version, 3)) {
    printf("Failed to parse request http version");
    goto exit;
  }
  /* parse host */
  line = strtok_r(save_line, newline_char, &save_line);
  if (!line || try_parse_field(line, space_char, &req->host, 2)) {
    printf("Failed to parse request host");
    goto exit;
  }
  /* parse user agent */
  line = strtok_r(save_line, newline_char, &save_line);
  if (!line || try_parse_field(line, space_char, &req->agent, 2)) {
    printf("Failed to parse request user agent");
    goto exit;
  }
  /* parse content */
  char *content_ptr = save_line + 2;
  req->content = strdup(content_ptr);
  if (!req->content) {
    printf("Failed to parse content");
    goto exit;
  }
  rc = 0;
exit:
  return rc;
}
int serialize_response(struct response *resp, char *buf, int size) {
  int index = 0;
  index += snprintf(buf, size - index, "%s\r\n",
                    server_status_responses[resp->status]);
  if (resp->content) {
    index += snprintf(&buf[index], size - index, "Content-Type: %s\r\n",
                      server_content_types[resp->type]);
    index += snprintf(&buf[index], size - index, "Content-Length: %ld\r\n\r\n",
                      strlen(resp->content));
    index += snprintf(&buf[index], size - index, "%s\r\n", resp->content);
  } else {
    index += snprintf(&buf[index], size - index, "\r\n");
  }
  return 0;
}
char recv_buffer[RECV_BUFFER_SIZE];
int main() {
  // Disable output buffering
  setbuf(stdout, NULL);
  // You can use print statements as follows for debugging, they'll be visible
  // when running tests.
  printf("Logs from your program will appear here!\n");
  ssize_t recv_len;
  int server_fd, client_addr_len, client_fd;
  struct sockaddr_in client_addr;
  struct request *req = malloc(sizeof(struct request));
  struct response *resp = malloc(sizeof(struct response));
  if (!req || !resp) {
    printf("Failed to alloc request struct");
    return 1;
  }
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    printf("Socket creation failed: %s...\n", strerror(errno));
    return 1;
  } 
  // setting REUSE_PORT ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) <
      0) {
    printf("SO_REUSEPORT failed: %s \n", strerror(errno));
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
  if (listen(server_fd, connection_backlog) != 0) {
    printf("Listen failed: %s \n", strerror(errno));
    return 1;
  }
  printf("Waiting for a client to connect...\n");
  client_addr_len = sizeof(client_addr);
  /* if any client desires to connect -- accept */
  client_fd =
      accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
  if (client_fd == -1) {
    printf("Failed to connect to client: %s\n", strerror(errno));
    return 1;
  }
  printf("Client connected\n");
  /* get any client data */
  recv_len = recv(client_fd, recv_buffer, RECV_BUFFER_SIZE, 0);
  printf("Got this from the client:\n%s\n", recv_buffer);
  /* parse client request */
  if (parse_request(recv_buffer, req)) {
    printf("Failed to parse the client request\n");
    return 1;
  }
  print_request(req);
  /* respond to the request */
  memset(resp, 0, sizeof(struct response));
  if (strcmp(req->path, "/") == 0) {
    resp->status = R_HTTP_OK;
  } else if (strstr(req->path, "echo")) {
    char *random_string = NULL, *echo = NULL;
    if (try_parse_field(req->path, slash_char, &echo, 1) ||
        try_parse_field(req->path, slash_char, &random_string, 2) ||
        strcmp(echo, "echo") != 0) {
      resp->status == R_NOT_FOUND;
    } else {
      resp->status = R_HTTP_OK;
      resp->content = random_string;
      resp->type = CT_TEXT_PLAIN;
    }
    if (echo)
      free(echo);
    if (resp->status != R_HTTP_OK && random_string)
      free(random_string);
  } else if (strcmp(req->path, "/user-agent") == 0) {
    resp->status = R_HTTP_OK;
    resp->content = strdup(req->agent);
    resp->type = CT_TEXT_PLAIN;
  } else {
    resp->status = R_NOT_FOUND;
  }
  if (serialize_response(resp, recv_buffer, RECV_BUFFER_SIZE)) {
    return 1;
  }
  printf("Response:\n%s", recv_buffer);
  if (send_response(client_fd, recv_buffer)) {
    return 1;
  }
  printf("Response sent!\n");
  close(server_fd);
  free_req(req);
  free_resp(resp);
  return 0;
}