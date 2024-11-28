#include "../include/http.h"
#include "../include/utils.h"
#include "../include/main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>

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
    printf("Client request:\n");
    printf("Method: %s, Path: %s, Protocol: %s\n", method, path, protocol);


    // 요청 메소드 GET인 경우 고려
    if (strcmp(method, "GET") == 0)
    {
        handle_get(cfd, path + 1); // GET 요청 처리
    }
    else if (strcmp(method, "HEAD") == 0)
    {
        handle_head(cfd, path + 1); // HEAD 요청 처리
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
    snprintf(path, sizeof(path), "file/%s", fname);

    FILE *fp = fopen(path, "rb");
    if (!fp)
    {
        response(cfd, 404, "Not Found", "text/plain", "File not found");
        return;
    }

    const char *mime_type = type(path);
    if (strncmp(mime_type, "image/", 6) == 0)
    {
        // 이미지 파일인 경우 청크 전송
        char header[512];
        snprintf(header, sizeof(header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: %s\r\n"
                 "Transfer-Encoding: chunked\r\n"
                 "\r\n",
                 mime_type);
        write(cfd, header, strlen(header));

        // 청크 단위로 파일 읽기 및 전송
        char buffer[4096];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0)
        {
            // 청크 크기 전송
            char chunk_header[32];
            snprintf(chunk_header, sizeof(chunk_header), "%zx\r\n", bytes_read);
            write(cfd, chunk_header, strlen(chunk_header));

            // 실제 데이터 전송
            write(cfd, buffer, bytes_read);

            // 청크 종료
            write(cfd, "\r\n", 2);
        }

        // 마지막 청크(종료 청크) 전송
        write(cfd, "0\r\n\r\n", 5);
    }
    else
    {
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        // 파일 크기만큼 메모리 할당
        char *buf = (char *)malloc(file_size);

        if (buf)
            fread(buf, 1, file_size, fp);
        fclose(fp);
        char header[512]; // 응답 헤더
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
}

void handle_head(int cfd, const char *fname)
{
    char path[100];
    snprintf(path, sizeof(path), "file/%s", fname);

    FILE *fp = fopen(path, "rb");
    if (!fp)
    {
        response(cfd, 404, "Not Found", "text/plain", NULL);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
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
}