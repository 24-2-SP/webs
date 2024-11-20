#include "../include/http.h"
#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

// 클라이언트 요청 처리
void handle_req(int cfd)
{
    char buf[4096];
    int bytes = read(cfd, buf, sizeof(buf) - 1);

    if (bytes <= 0)
    {
        perror("read failed");
        close(cfd);
        return;
    }
    buf[bytes] = '\0';

    // 요청 파싱
    char method[16], path[256], protocol[16];
    sscanf(buf, "%s %s %s", method, path, protocol);

    // 요청 메소드 GET인 경우만 고려
    if (strcmp(method, "GET") == 0)
    {
        // 맨앞의 '/'를 제거한 경로 전달
        handle_get(cfd, path + 1);
    }
    else
    // GET 메소드가 아닌 경우
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

// http 응답 -> 클라이언트로 전송
void response(int cfd, int status, const char *statusM, const char *types, const char *body)
{
    char buf[4096];
    // 응답 header&body
    snprintf(buf, sizeof(buf),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "\r\n"
             "%s",
             status, statusM, types, strlen(body), body);
    // buf 내용 -> 클라이언트 소켓 (cfd)
    write(cfd, buf, strlen(buf));
}
