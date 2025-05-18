#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "request.h"
#include "config.h"


void print_request(struct request *req) {
    printf("Request object:\n");
    printf("\t path: %s\n", req->path);
    printf("\t version: %s\n", req->http_version);
    printf("\t host: %s\n", req->host);
    printf("\t agent: %s\n", req->agent);
    printf("\t content: %s\n", req->content);
}

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
  
int parse_request(char *buf, size_t buf_len, struct request *req) {
    memset(req, 0, sizeof(struct request));

    // Find the end of headers (double CRLF)
    char *headers_end = strstr(buf, "\r\n\r\n");
    if (!headers_end) {
        return -1; // Headers not complete yet
    }

    size_t headers_len = headers_end - buf + 4; // Include "\r\n\r\n"
    char *headers = strndup(buf, headers_len);
    if (!headers) return -1;

    // Begin parsing line by line
    char *save_line;
    char *line = strtok_r(headers, newline_char, &save_line);
    
    // Parse request line
    if (!line) return -1;
    char* tmp;
    tmp = strtok(line, " ");
    req->method = tmp ? strdup(tmp) : NULL;
    tmp = strtok(NULL, " ");
    req->path = tmp ? strdup(tmp) : NULL;
    tmp = strtok(NULL, " ");
    req->http_version = tmp ? strdup(tmp) : NULL;
    if (!req->method || !req->path || !req->http_version) return -1;

    // Parse headers
    while ((line = strtok_r(NULL, newline_char, &save_line))) {
        if (strncasecmp(line, "Host:", 5) == 0) {
            req->host = strdup(line + 6);
        } else if (strncasecmp(line, "User-Agent:", 11) == 0) {
            req->agent = strdup(line + 12);
        } else if (strncasecmp(line, "Content-Length:", 15) == 0) {
            req->content_length = atoi(line + 16);
        }
    }

    // Parse body (after header end)
    size_t body_offset = headers_len;
    size_t remaining = buf_len - body_offset;
    if (req->content_length > 0) {
        if (remaining < req->content_length) {
            return -2; // Need more data
        }
        req->content = (char *)malloc(req->content_length + 1);
        if (!req->content) return -1;
        memcpy(req->content, buf + body_offset, req->content_length);
        req->content[req->content_length] = '\0';
    }

    return headers_len + req->content_length; // total bytes consumed
}
  
int recieve_request(int client_fd, struct request *req) {
    char *save_line, *line, *field;
    int cntr, total_received = 0, buffer_size = RECV_BUFFER_SIZE;
    char *recv_buffer = malloc(buffer_size);
    memset(req, 0, sizeof(struct request));
    while (1) {
        // Grow buffer if needed
        if (buffer_size - total_received < 1024) {
            buffer_size *= 2;
            recv_buffer = realloc(recv_buffer, buffer_size);
        }

        int recv_len = recv(client_fd, recv_buffer + total_received, buffer_size - total_received, 0);
        if (recv_len <= 0) {
            printf("Client disconnected or error.\n");
            break;
        }
        total_received += recv_len;

        int parse_result = parse_request(recv_buffer, total_received, req);
        if (parse_result == -1 || parse_result == -2) {
            continue; // Need more data (headers or body incomplete)
        }
        break;
    }
  
    return 0;
}