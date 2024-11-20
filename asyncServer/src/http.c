#include "http.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void handle_client_request(int cfd, int epoll_fd)
{
    char buf[BUFFER_SIZE];
    int bytes = read(cfd, buf, sizeof(buf) - 1);

    if (bytes <= 0)
    {
        close(cfd);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cfd, NULL);
    }
    else
    {
        buf[bytes] = '\0';
        handle_req(cfd, buf);
    }
}

void handle_req(int cfd, const char *buf)
{
    char method[16], path[256], protocol[16];
    sscanf(buf, "%s %s %s", method, path, protocol);

    if (strcmp(method, "GET") == 0)
    {
        handle_get(cfd, path + 1);
    }
    else
    {
        const char *body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
        response(cfd, 405, "Method Not Allowed", "text/html", body);
    }
}

void handle_get(int cfd, const char *fname)
{
    char path[100];
    snprintf(path, sizeof(path), "../file/%s", fname);

    FILE *fp = fopen(path, "rb");
    if (!fp)
    {
        response(cfd, 404, "Not Found", "text/plain", "File not found");
        return;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buf = (char *)malloc(file_size);
    if (buf)
        fread(buf, 1, file_size, fp);
    fclose(fp);

    const char *mime_type = type(path);

    char header[512];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "\r\n",
             mime_type, file_size);
    write(cfd, header, strlen(header));
    write(cfd, buf, file_size);
    free(buf);
}