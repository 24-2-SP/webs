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

    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        response(cfd, 404, "Not Found", "text/plain", "File not found");
        return;
    }

    // 파일 정보 가져오기
    struct stat st;
    if (fstat(fd, &st) == -1)
    {
        perror("fstat failed");
        close(fd);
        response(cfd, 500, "Internal Server Error", "text/plain", "Unable to process file");
        return;
    }

    const char *mime_type = type(path);

    // 이미지 파일인 경우
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
        write(cfd, header, strlen(header));

        // 파일을 메모리 매핑
        char *mapped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (mapped == MAP_FAILED)
        {
            perror("mmap failed");
            close(fd);
            response(cfd, 500, "Internal Server Error", "text/plain", "Unable to process file");
            return;
        }

        // 청크 단위로 전송
        size_t remaining = st.st_size;
        char *current = mapped;
        char chunk_header[64];

        while (remaining > 0)
        {
            size_t chunk_size = (remaining > 4096) ? 4096 : remaining;

            // 청크 헤더 전송
            int header_size = snprintf(chunk_header, sizeof(chunk_header), "%zx\r\n", chunk_size);
            write(cfd, chunk_header, header_size);

            // 청크 데이터 전송
            write(cfd, current, chunk_size);
            write(cfd, "\r\n", 2);

            current += chunk_size;
            remaining -= chunk_size;
        }

        // 마지막 청크(종료 청크) 전송
        write(cfd, "0\r\n\r\n", 5);

        // 메모리 매핑 해제
        munmap(mapped, st.st_size);
    }
    else
    {
        // 일반 파일 전송
        char *mapped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (mapped == MAP_FAILED)
        {
            perror("mmap failed");
            close(fd);
            response(cfd, 500, "Internal Server Error", "text/plain", "Unable to process file");
            return;
        }

        char header[512];
        snprintf(header, sizeof(header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: %s\r\n"
                 "Content-Length: %ld\r\n"
                 "\r\n",
                 mime_type, st.st_size);
        write(cfd, header, strlen(header));

        // 파일 데이터 전송
        write(cfd, mapped, st.st_size);

        // 메모리 매핑 해제
        munmap(mapped, st.st_size);
    }

    close(fd);
}

// //void handle_get(int cfd, const char *fname)
// {
//     char path[100];
//     snprintf(path, sizeof(path), "file/%s", fname);

//     FILE *fp = fopen(path, "rb");
//     if (!fp)
//     {
//         response(cfd, 404, "Not Found", "text/plain", "File not found");
//         return;
//     }

//     const char *mime_type = type(path);
//     if (strncmp(mime_type, "image/", 6) == 0)
//     {
//         // 이미지 파일인 경우 청크 전송
//         char header[512];
//         snprintf(header, sizeof(header),
//                  "HTTP/1.1 200 OK\r\n"
//                  "Content-Type: %s\r\n"
//                  "Transfer-Encoding: chunked\r\n"
//                  "\r\n",
//                  mime_type);
//         write(cfd, header, strlen(header));

//         // 청크 단위로 파일 읽기 및 전송
//         char buffer[4096];
//         size_t bytes_read;
//         char chunk_header[64];

//         while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0)
//         {
//             // 청크 크기 전송
//             int needed_size = snprintf(chunk_header, sizeof(chunk_header), "%zx\r\n", bytes_read);
//             if (needed_size >= sizeof(chunk_header))
//             {
//                 fprintf(stderr, "Error: Chunk header size exceeded\n");
//                 fclose(fp);
//                 close(cfd);
//                 return;
//             }
//             write(cfd, chunk_header, needed_size);
//             write(cfd, buffer, bytes_read);
//             write(cfd, "\r\n", 2);
//         }

//         // 마지막 청크(종료 청크) 전송
//         write(cfd, "0\r\n\r\n", 5);
//         fclose(fp);
//     }
//     else
//     {
//         fseek(fp, 0, SEEK_END);
//         long file_size = ftell(fp);
//         fseek(fp, 0, SEEK_SET);

//         // 파일 크기만큼 메모리 할당
//         char *buf = (char *)malloc(file_size);

//         if (buf)
//             fread(buf, 1, file_size, fp);
//         fclose(fp);
//         char header[512]; // 응답 헤더
//         snprintf(header, sizeof(header),
//                  "HTTP/1.1 200 OK\r\n"
//                  "Content-Type: %s\r\n"
//                  "Content-Length: %ld\r\n"
//                  "\r\n",
//                  mime_type, file_size);
//         write(cfd, header, strlen(header));

//         write(cfd, buf, file_size);
//         free(buf);
//     }
// }

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