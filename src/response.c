#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "response.h"

const char *server_status_responses[R_COUNT] = {
    [R_HTTP_OK] = "HTTP/1.1 200 OK",
    [R_NOT_FOUND] = "HTTP/1.1 404 Not Found",
};

const char *server_content_types[CT_COUNT] = {
    [CT_TEXT_PLAIN] = "text/plain",
    [CT_OCTET_STREAM] = "application/octet-stream",
    [CT_TEXT_HTML] = "text/html",
    [CT_STYLE_CSS] = "text/css",
    [CT_APPLICATION_JAVASCRIPT] = "application/javascript",
    [CT_APPLICATION_JSON] = "application/json",
    [CT_IMAGE_JPEG] = "image/jpeg",
    [CT_IMAGE_PNG] = "image/png",
    [CT_IMAGE_GIF] = "image/gif",
    [CT_IMAGE_SVG] = "image/svg+xml",
    [CT_APPLICATION_PDF] = "application/pdf"
};

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
    int header_len = serialize_response(resp, header_buf, sizeof(header_buf));
    if (header_len < 0) return -1;
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