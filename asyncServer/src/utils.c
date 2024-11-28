#include "../include/utils.h"
#include "../include/main.h"
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

void set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl get failed");
        exit(1);
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl set failed");
        exit(1);
    }
}

const char *type(const char *fname) {
    const char *ext = strrchr(fname, '.');
    if (!ext || ext == fname) return "application/octet-stream";

    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".txt") == 0) return "text/plain";
    if (strcmp(ext, ".html") == 0) return "text/html";

    return "application/octet-stream";
}