// 비동기 http server
#include "../include/server.h"
#include "../include/utils.h"
#include "../include/http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>


// HTTP 응답 작성 및 전송
void response(int cfd, int status, const char *statusM, const char *types, const char *body)
{
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

// 서버 초기화
int init_server()
{
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1)
    {
        perror("socket failed");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("setsockopt failed");
        close(sfd);
        exit(1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind failed");
        close(sfd);
        exit(1);
    }

    if (listen(sfd, 1000) == -1)
    {
        perror("listen failed");
        close(sfd);
        exit(1);
    }

    printf("Server is running on port 12345\n");
    return sfd;
}

void handle_connection(int sfd, int epoll_fd)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int cfd = accept(sfd, (struct sockaddr *)&client_addr, &client_len);
    if (cfd == -1)
    {
        perror("accept failed");
        exit(1);
    }

    printf("New client connected: %d\n", cfd);
    set_non_blocking(cfd);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET; // Edge-triggered 읽기 이벤트
    ev.data.fd = cfd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cfd, &ev) == -1)
    {
        perror("epoll_ctl failed");
        close(cfd);
    }
}