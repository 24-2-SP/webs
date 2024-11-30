#include "../include/main.h"


ssize_t send_data(int cfd, const char *data, size_t length)
{
    size_t total_sent = 0;

    while (total_sent < length)
    {
        ssize_t sent = write(cfd, data + total_sent, length - total_sent);
        if (sent == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 일시적 오류 - 재시도
                continue;
            }
            else
            {
                // 다른 오류 발생
                perror("Write failed");
                return -1;
            }
        }
        total_sent += sent; // 총 전송된 데이터 크기 증가
    }

    return total_sent;
}

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
