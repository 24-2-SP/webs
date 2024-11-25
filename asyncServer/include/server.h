#ifndef SERVER_H
#define SERVER_H

#include <sys/epoll.h>

// 서버 초기화 함수 (포트를 매개변수로 받음)
int init_server();

// 새로운 클라이언트 연결 처리
void handle_connection(int sfd, int epoll_fd);

void response(int cfd, int status, const char *statusM, const char *types, const char *body);

#endif