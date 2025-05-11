#include <stdlib.h>
#include <stdio.h>
#include "config.h"

const char *newline_char = "\r\n";
const char *space_char = " ";
const char *slash_char = "/";

int get_file_fd(char* filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen");
        return -1; // indicate error
    }
  
    int fd = fileno(fp);
    
    return fd;
}