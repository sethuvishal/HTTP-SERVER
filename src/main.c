#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#define RECV_BUFFER_SIZE 10000
typedef enum {
  R_HTTP_OK = 0,
  R_NOT_FOUND,
  /* guard */
  R_COUNT
} resp_status_e;
typedef enum {
  CT_TEXT_PLAIN = 0,
  CT_OCTET_STREAM,
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
  int has_content_length;
  long content_length;
  char *content;
};
char *server_status_responses[R_COUNT] = {
    [R_HTTP_OK] = "HTTP/1.1 200 OK",
    [R_NOT_FOUND] = "HTTP/1.1 404 Not Found",
};
char *server_content_types[CT_COUNT] = {
    [CT_TEXT_PLAIN] = "text/plain",
    [CT_OCTET_STREAM] = "application/octet-stream",
};
char *newline_char = "\r\n";
char *space_char = " ";
char *slash_char = "/";
// request handlers
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

void print_request(struct request *req) {
  printf("Request object:\n");
  printf("\t path: %s\n", req->path);
  printf("\t version: %s\n", req->http_version);
  printf("\t host: %s\n", req->host);
  printf("\t agent: %s\n", req->agent);
  printf("\t content: %s\n", req->content);
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
  memset(req, 0, sizeof(struct request));
  /* parse path */
  line = strtok_r(buf, newline_char, &save_line);
  if (!line || try_parse_field(line, space_char, &req->path, 2)) {
    printf("Failed to parse request path");
    return 0;
  }
  /* parse http version */
  if (!line || try_parse_field(line, space_char, &req->http_version, 3)) {
    printf("Failed to parse request http version");
    return 0;
  }
  /* parse host */
  line = strtok_r(save_line, newline_char, &save_line);
  if (!line || try_parse_field(line, space_char, &req->host, 2)) {
    printf("Failed to parse request host");
	return 0;
  }
  /* parse user agent */
  line = strtok_r(save_line, newline_char, &save_line);
  if (!line || try_parse_field(line, space_char, &req->agent, 2)) {
    printf("Failed to parse request user agent");
	return 0;
  }
  /* parse content */
  char *content_ptr = save_line + 2;
  req->content = strdup(content_ptr);
  if (!req->content) {
    printf("Failed to parse content");
  }
  return 0;
}

// response handlers
typedef int (*stream_callback)(int cfd);
void free_resp(struct response *resp) {
  if (resp->content)
    free(resp->content);
  free(resp);
}

int send_all(int cfd, const char *data, size_t len) {
  ssize_t sent_bytes, remaining = len;
  while (remaining) {
    sent_bytes = send(cfd, data + (len - remaining), remaining, 0);
    if (sent_bytes < 0) {
      perror("send");
      return -1;
    }
    remaining -= sent_bytes;
  }
  return 0;
}

int serialize_response(struct response *resp, char *buf, int size) {
  if(resp->content != NULL){
    resp->has_content_length = 1;
    resp->content_length = strlen(resp->content);
  }
  int index = 0;
  index += snprintf(buf, size - index, "%s\r\n", server_status_responses[resp->status]);

  if (resp->has_content_length) {
    index += snprintf(&buf[index], size - index, "Content-Type: %s\r\n",
                      server_content_types[resp->type]);
    index += snprintf(&buf[index], size - index, "Content-Length: %ld\r\n", resp->content_length);
  } else {
    index += snprintf(&buf[index], size - index, "Transfer-Encoding: chunked\r\n");
  }

  index += snprintf(&buf[index], size - index, "\r\n");
  return index;
}

int send_response(int cfd, struct response *resp, int src_fd) {
  char header_buf[1024];
  printf("Sending Response \n");
  int header_len = serialize_response(resp, header_buf, sizeof(header_buf));
  printf("Header Length: %d", header_len);
  if (header_len < 0) return -1;
  printf("Headers Serealized");
  // Send headers
  if (send_all(cfd, header_buf, header_len) != 0) {
    fprintf(stderr, "Failed to send headers\n");
    return -1;
  }

  if(resp->content){
    send_all(cfd, resp->content, strlen(resp->content));
  }

  // Stream body (if present)
  if(src_fd == 0) return 0;

  if (stream_fd_to_client(src_fd, cfd) != 0) {
    fprintf(stderr, "Failed to stream response body\n");
    return -1;
  }

  return 0;
}

int stream_fd_to_client(int src_fd, int cfd) {
  char buffer[4096];
  ssize_t n_read, n_sent;

  while ((n_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
    ssize_t total_sent = 0;
    while (total_sent < n_read) {
      n_sent = send(cfd, buffer + total_sent, n_read - total_sent, 0);
      if (n_sent < 0) {
        perror("send");
        return -1;
      }
      total_sent += n_sent;
    }
  }

  if (n_read < 0) {
    perror("read");
    return -1;
  }

  return 0;
}

int get_file_fd(char* filename) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
      perror("fopen");
      return -1; // indicate error
  }

  int fd = fileno(fp);
  
  return fd;
}

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
    resp->content = "Something went wrong!";
    resp->type = CT_TEXT_PLAIN;
    return resp;
  }
  snprintf(fullpath, sizeof(fullpath), "%s/%s", cwd, filename);
  // Check if file exists
  if (access(fullpath, F_OK) != 0) {
      printf("File not found\n");
      resp->status = R_NOT_FOUND;
      resp->content = "File not found";
      resp->type = CT_TEXT_PLAIN;
      return resp;
  }

  FILE *fp = fopen(fullpath, "r");
  fseek(fp, 0, SEEK_END);
  long filesize = ftell(fp);
  fclose(fp);
  resp->status = R_HTTP_OK;
  resp->type = CT_TEXT_PLAIN;
  resp->has_content_length = 1;
  resp->content_length = filesize;
  resp->content = NULL;

  return resp;
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
      close(client_fd);
      continue;
    }
    if (client_fd == -1) {
      printf("Failed to connect to client: %s\n", strerror(errno));
    }
    printf("Client connected\n");
    /* get any client data */
    recv_len = recv(client_fd, recv_buffer, RECV_BUFFER_SIZE, 0);
    printf("Got this from the client:\n%s\n", recv_buffer);
    /* parse client request */
    if (parse_request(recv_buffer, req)) {
      printf("Failed to parse the client request\n");
    }
    /* respond to the request */
    printf("creating response object");
    memset(resp, 0, sizeof(struct response));
    if (strcmp(req->path, "/") == 0) {
      resp->status = R_HTTP_OK;
      send_response(client_fd, resp, NULL);
    } else if (strstr(req->path, "echo")) {
      resp = echo_handler(req);
      send_response(client_fd, resp, NULL);
    } else if (strcmp(req->path, "/user-agent") == 0) {
      resp->status = R_HTTP_OK;
      resp->content = strdup(req->agent);
      resp->type = CT_TEXT_PLAIN;
      send_response(client_fd, resp, NULL);
    } else if (strncmp(req->path, "/files/", 7) == 0) {
      resp = serve_file(req);
      if(resp->status == R_HTTP_OK){
        int fd = get_file_fd(&req->path[strlen("/files/")]);
        send_response(client_fd, resp, fd == -1 ? 0 : fd);
      }else{
        send_response(client_fd, resp, 0);
      }
    }else {
      resp->status = R_NOT_FOUND;
    }

    printf("Response sent!\n");
    free_req(req);
    printf("request cleared\n");
    free_resp(resp);
    printf("response cleared\n");
    if(getpid() != main_pid){
      exit(0);
    }
  }
  close(server_fd);
  return 0;
}