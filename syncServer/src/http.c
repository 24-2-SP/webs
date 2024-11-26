#include "../include/main.h"


// 클라이언트 요청 처리
void handle_req(int cfd)
{
    char buf[BUFFER_SIZE];
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

    //파일 크기만큼 메모리 할당 
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
    // char buf[BUFFER_SIZE];
    
    // // 응답 header&body
    // snprintf(buf, sizeof(buf),
    //          "HTTP/1.1 %d %s\r\n"
    //          "Content-Type: %s\r\n"
    //          "Content-Length: %ld\r\n"
    //          "\r\n"
    //          "%s",
    //          status, statusM, types, strlen(body), body);
    // // buf 내용 -> 클라이언트 소켓 (cfd)
    // write(cfd, buf, strlen(buf));


    //snprintf 동적으로 버퍼 크기 확장
    char *buf = (char *)malloc(BUFFER_SIZE);
    if (!buf)
    {
        perror("malloc failed");
        close(cfd);
        return;
    }
    int needed_size = snprintf(buf, BUFFER_SIZE,
                                "HTTP/1.1 %d %s\r\n"
                                "Content-Type: %s\r\n"
                                "Content-Length: %ld\r\n"
                                "\r\n"
                                "%s",
                                status, statusM, types, strlen(body), body);

    // 반환된 크기가 현재 버퍼 크기를 초과하는 경우
    if (needed_size >= BUFFER_SIZE)
    {
        size_t size = needed_size + 1; // null-terminator 포함
        buf = (char *)realloc(buf, size);
        if (!buf)
        {
            perror("realloc failed");
            close(cfd);
            return;
        }

        // 다시 작성
        snprintf(buf, size,
                 "HTTP/1.1 %d %s\r\n"
                 "Content-Type: %s\r\n"
                 "Content-Length: %ld\r\n"
                 "\r\n"
                 "%s",
                 status, statusM, types, strlen(body), body);
    }

    // 클라이언트로 데이터 전송
    write(cfd, buf, strlen(buf));
    free(buf);

}
