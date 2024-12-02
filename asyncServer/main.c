#include "./include/main.h"

int main()
{
    int sfd = init_server();
    set_non_blocking(sfd); // 서버 소켓을 비차단 모드로 설정

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        perror("epoll_create1 failed");
        close(sfd);
        exit(1);
    }

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN; // 읽기 이벤트 감지
    ev.data.fd = sfd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sfd, &ev) == -1)
    {
        perror("epoll_ctl failed");
        close(sfd);
        close(epoll_fd);
        exit(1);
    }

    while (1)
    {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1)
        {
            perror("epoll_wait failed");
            break;
        }

        for (int i = 0; i < num_events; i++)
        {
            if (events[i].events & (EPOLLHUP | EPOLLERR))
            {
                // 연결 종료 또는 에러 발생
                perror("Client hang-up or error");
                close_connection(events[i].data.fd, epoll_fd);
                continue;
            }
            if (events[i].data.fd == sfd)
            { // 새로운 연결 처리
                handle_connection(sfd, epoll_fd);
            }
            else if (events[i].events & EPOLLIN)
            { // 요청 처리
                handle_client_request(events[i].data.fd, epoll_fd);
            }
        }
    }

    close(sfd);
    close(epoll_fd);
    return 0;
}
