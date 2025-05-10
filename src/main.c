#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
typedef uint64_t u64;
typedef int64_t i64;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint8_t u8;
typedef int8_t i8;
typedef float f32;
typedef double f64;

enum http_request {
	HTTP_REQUEST_GET,
	HTTP_REQUEST_POST,
	HTTP_REQUEST_PUT,
	HTTP_REQUEST_PATCH,
	HTTP_REQUEST_DELETE,
	HTTP_REQUEST__COUNT,
};

struct method_string_pair {
	enum http_request method;
	u8 *str;
} known_methods[] = {
	{HTTP_REQUEST_GET, "GET"},       {HTTP_REQUEST_POST, "POST"},
	{HTTP_REQUEST_PUT, "PUT"},       {HTTP_REQUEST_PATCH, "PATCH"},
	{HTTP_REQUEST_DELETE, "DELETE"},
};

struct http_header {
	enum http_request method;
	u8 *path;
	u8 user_agent[1024];
  	u8 content_type[1024];
	int content_length;
};

enum http_request method_from_str(u8 *str) {
	for (u32 i = 0; i < sizeof(known_methods); i++)
	  if (strcmp(str, known_methods[i].str) == 0)
		return known_methods[i].method;
	return -1;
}

int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

	int server_fd, client_addr_len;
	struct sockaddr_in client_addr;
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}
	
	// setting SO_REUSEADDR ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1;
	}
	
	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
									 .sin_port = htons(4221),
									 .sin_addr = { htonl(INADDR_ANY) },
									};
	
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}
	
	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}
	
	printf("Waiting for a client to connect...\n");
	client_addr_len = sizeof(client_addr);
	int new_socket;
	
	new_socket = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
	if (new_socket < 0) {
		perror("accept");
		exit(EXIT_FAILURE);
	}
	printf("Client connected\n");

	u8 buffer[1024];
	read(new_socket, buffer, sizeof(buffer));

	struct http_header request = {0};
	request.method = method_from_str(strtok(buffer, " "));
	request.path = strtok(0, " ");

	printf("path: %s\n method: %d\n", request.path, request.method);
	if (strcmp(request.path, "/") == 0) {
		char ok_200[] = "HTTP/1.1 200 OK\r\n\r\n";
		send(new_socket, ok_200, sizeof(ok_200), 0);
	} else if(strcmp(strtok(request.path, "/"), "echo") == 0){
		char* path = strtok(0, "/");
		char response[2048];
		sprintf(response,
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: %ld\r\n"
			"\r\n"
			"%s",
			strlen(path),
			path
		);
		send(new_socket, response, sizeof(response), 0);
	} else {
		char not_found_404[] = "HTTP/1.1 404 NOT FOUND\r\n\r\n";
		send(new_socket, not_found_404, sizeof(not_found_404), 0);
	}


	close(new_socket);
	close(server_fd);

	return 0;
}
