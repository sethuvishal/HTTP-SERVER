#include <string.h>
#include "response.h"

const char* get_file_extension(const char* filename) {
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        return "";
    }
    return dot + 1;
}

content_type_e get_content_type(const char* ext) {
    if(!ext){
        return CT_OCTET_STREAM;
    }

    if (strcmp(ext, "html") == 0 || strcmp(ext, "htm") == 0) {
        return CT_TEXT_HTML;
    } else if (strcmp(ext, "css") == 0) {
        return CT_STYLE_CSS;
    } else if (strcmp(ext, "js") == 0) {
        return CT_APPLICATION_JAVASCRIPT;
    } else if (strcmp(ext, "json") == 0) {
        return CT_APPLICATION_JSON;
    } else if (strcmp(ext, "png") == 0) {
        return CT_IMAGE_PNG;
    } else if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0) {
        return CT_IMAGE_JPEG;
    } else if (strcmp(ext, "gif") == 0) {
        return CT_IMAGE_GIF;
    } else if (strcmp(ext, "svg") == 0) {
        return CT_IMAGE_SVG;
    } else if (strcmp(ext, "txt") == 0) {
        return CT_TEXT_PLAIN;
    } else if (strcmp(ext, "pdf") == 0) {
        return CT_APPLICATION_PDF;
    } else {
        return CT_OCTET_STREAM; // Fallback for unknown types
    }
}