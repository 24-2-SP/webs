#include "../include/main.h"

// HTTP 응답 작성 및 전송
void response(int cfd, int status, const char *statusM, const char *types, const char *body)
{
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
        char *new_buf = (char *)realloc(buf, size);
        if (!new_buf)
        {
            perror("realloc failed");
	    free(buf);
            close(cfd);
            return;
        }
	buf=new_buf;
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
    if(write(cfd, buf, strlen(buf))==-1){
	perror("write failed");
    }
    free(buf);
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
    server_addr.sin_port = htons(PORT);
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

    printf("Server is running on port %d\n", PORT);
    return sfd;
}

atomic_int active_connections = 0;
void handle_connection(int sfd, int epoll_fd)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int cfd = accept(sfd, (struct sockaddr *)&client_addr, &client_len);
    if (cfd == -1)
    {
        perror("accept failed");
        return;
    }

    // 동시 연결 수 증가
    atomic_fetch_add(&active_connections, 1);
    printf("New client connected: %d\n", cfd);
    printf("Active connections: %d\n", atomic_load(&active_connections));

    set_non_blocking(cfd);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET; // Edge-triggered 읽기 이벤트
    ev.data.fd = cfd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cfd, &ev) == -1)
    {
        perror("epoll_ctl failed");
        close(cfd);

        // 동시 연결 수 감소 (epoll 등록 실패 시)
        atomic_fetch_sub(&active_connections, 1);
    }
}

// 연결 종료 시 동시 연결 수 감소
void close_connection(int cfd, int epoll_fd)
{
    printf("Closing connection: %d\n", cfd);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cfd, NULL); // epoll에서 소켓 제거
    close(cfd); // 소켓 닫기

    // 동시 연결 수 감소
    atomic_fetch_sub(&active_connections, 1);
    printf("Active connections: %d\n", atomic_load(&active_connections));
} 
