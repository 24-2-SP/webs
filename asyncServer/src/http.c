#include "../include/main.h"

void handle_client_request(int cfd, int epoll_fd)
{
    char buf[BUFFER_SIZE];

    // 클라이언트로부터 데이터 읽기
    ssize_t bytes_read = read(cfd, buf, sizeof(buf) - 1);
    if (bytes_read == 0)
    {
        // 클라이언트가 연결을 종료했음
        printf("Client disconnected: %d\n", cfd);
        close_connection(cfd, epoll_fd);
        return;
    }

    if (bytes_read <= 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // 더 이상 읽을 데이터 없음
            return;
        }
        else
        {
            perror("Read failed");
            close_connection(cfd, epoll_fd);
            return;
        }
    }

    // 읽은 데이터를 null-terminate하여 문자열 처리
    buf[bytes_read] = '\0';
    printf("Request: %s\n", buf);

    // 요청 URL 추출
    char *path_start = strchr(buf, ' ');
    if (!path_start)
    {
        response(cfd, 400, "Bad Request", "text/plain", "Invalid Request");
        close_connection(cfd, epoll_fd);
        return;
    }
    path_start++;

    char *path_end = strchr(path_start, ' '); // 다음 ' '에서 끝남
    if (!path_end)
    {
        response(cfd, 400, "Bad Request", "text/plain", "Invalid Request");
        close_connection(cfd, epoll_fd);
        return;
    }
    *path_end = '\0';
    //요청 메소드 처리
    if (strncmp(buf, "GET", 3) == 0)
    {
        handle_get(cfd, path_start);
    }
    else if (strncmp(buf, "HEAD", 4) == 0)
    {
        handle_head(cfd, path_start);
    }
    else
    {
        response(cfd, 400, "Bad Request", "text/plain", "Invalid Request");
    }

    close_connection(cfd, epoll_fd); //연결 종료
}


void handle_get(int cfd, const char *fname)
//이미지와 일반파일 나눠서 처리
{
    char path[256];
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
        // HTTP 응답 헤더 전송
        char header[512];
        snprintf(header, sizeof(header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: %s\r\n"
                 "Transfer-Encoding: chunked\r\n"
                 "\r\n",
                 mime_type);
        if (write(cfd, header, strlen(header)) == -1)
        {
            perror("Failed to send header");
            fclose(fp);
            close(cfd);
            return;
        }

        // 청크 단위로 파일 읽기 및 전송
        char buffer[CHUNK_SIZE];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0)
        {
            // 청크 크기 전송
            char chunk_header[32];
            int header_len = snprintf(chunk_header, sizeof(chunk_header), "%zx\r\n", bytes_read);

            if (send_data(cfd, chunk_header, header_len) == -1 || send_data(cfd, buffer, bytes_read) == -1 || send_data(cfd, "\r\n", 2) == -1)
            {
                perror("Failed to send chunk data");
                break;
            }
        }

        if (send_data(cfd, "0\r\n\r\n", 5) == -1)
        {
            perror("Failed to send chunk data");
        }
    }
    else
    {
        // 일반 파일 처리 (전체 전송)
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        char *buf = (char *)malloc(file_size);
        if (!buf)
        {
            perror("Memory allocation failed");
            fclose(fp);
            response(cfd, 500, "Internal Server Error", "text/plain", "Unable to allocate memory");
            return;
        }

        fread(buf, 1, file_size, fp);
        fclose(fp);

        // HTTP 헤더 전송
        char header[512];
        snprintf(header, sizeof(header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: %s\r\n"
                 "Content-Length: %ld\r\n"
                 "\r\n",
                 mime_type, file_size);

        if (send_data(cfd, header, strlen(header)) == -1 ||
            send_data(cfd, buf, file_size) == -1)
        {
            perror("Failed to send file data");
        }
        free(buf);
    }
}

void handle_head(int cfd, const char *fname)
{
    //헤더만 전송
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
