//비동기 http server
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#define MAX_EVENTS 1024
#define BUFFER_SIZE 4096
#define PORT 12345

void response(int cfd, int status, const char *statusM, const char *types, const char *body);
void handle_req(int cfd, const char *buf);
const char *type(const char *fname);
void handle_get(int cfd, const char *fname);

// 소켓을 비차단(non-blocking) 모드로 설정
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

// HTTP 응답 작성 및 전송
void response(int cfd, int status, const char *statusM, const char *types, const char *body) {
    char buf[BUFFER_SIZE];
    snprintf(buf, sizeof(buf),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "\r\n"
             "%s",
             status, statusM, types, strlen(body), body);
    write(cfd, buf, strlen(buf));
}

// 클라이언트 요청 처리
void handle_req(int cfd, const char *buf) {
    char method[16], path[256], protocol[16];
    sscanf(buf, "%s %s %s", method, path, protocol);

    if (strcmp(method, "GET") == 0) {
        handle_get(cfd, path + 1); // '/' 제거 후 파일 경로 전달
    } else {
        const char *body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
        response(cfd, 405, "Method Not Allowed", "text/html", body);
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

// GET 요청 처리
void handle_get(int cfd, const char *fname) {
    char path[100];
    snprintf(path, sizeof(file_path), "../../file/%s", fname); // "file/" 경로로 설정

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        response(cfd, 404, "Not Found", "text/plain", "File not found");
        return;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buf = (char *)malloc(file_size);
    if (buf) fread(buf, 1, file_size, fp);
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

// 서버 초기화
int init_server() {
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
        perror("socket failed");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        close(sfd);
        exit(1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        close(sfd);
        exit(1);
    }

    if (listen(sfd, 1000) == -1) {
        perror("listen failed");
        close(sfd);
        exit(1);
    }

    printf("Server is running on port %d\n", PORT);
    return sfd;
}

int main () {
    int sfd = init_server();
    set_non_blocking(sfd); // 서버 소켓을 비차단 모드로 설정

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
        close(sfd);
        exit(1);
    }

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN; // 읽기 이벤트 감지
    ev.data.fd = sfd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sfd, &ev) == -1) {
        perror("epoll_ctl failed");
        close(sfd);
        close(epoll_fd);
        exit(1);
    }

    while (1) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1) {
            perror("epoll_wait failed");
            break;
        }

        for (int i = 0; i < num_events; i++) {
            if (events[i].data.fd == sfd) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int cfd = accept(sfd, (struct sockaddr *)&client_addr, &client_len);
                if (cfd == -1) {
                    perror("accept failed");
                    continue;
                }

                printf("New client connected: %d\n", cfd);
                set_non_blocking(cfd);

                ev.events = EPOLLIN | EPOLLET; // Edge-triggered 읽기 이벤트
                ev.data.fd = cfd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cfd, &ev) == -1) {
                    perror("epoll_ctl failed");
                    close(cfd);
                }
            } else if (events[i].events & EPOLLIN) {
                char buf[BUFFER_SIZE];
                int cfd = events[i].data.fd;
                int bytes = read(cfd, buf, sizeof(buf) - 1);

                if (bytes <= 0) {
                    close(cfd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cfd, NULL);
                } else {
                    buf[bytes] = '\0';
                    handle_req(cfd, buf);
                }
            }
        }
    }

    close(sfd);
    close(epoll_fd);
    return 0;
}