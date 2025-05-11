#ifndef CONFIG_H
#define CONFIG_H

#define RECV_BUFFER_SIZE 10000
extern const char *newline_char;
extern const char *space_char;
extern const char *slash_char;

int get_file_fd(char* filename) ;

#endif