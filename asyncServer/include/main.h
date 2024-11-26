#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>

// 서버 설정
#define PORT 12345
#define MAX_EVENTS 1024
#define BUFFER_SIZE 512

// 함수 선언
void handle_connection(int sfd, int epoll_fd);
void handle_req(int cfd, const char *buf);
void handle_get(int cfd, const char *fname);
void response(int cfd, int status, const char *statusM, const char *types, const char *body);
const char *type(const char *fname);
void set_non_blocking(int fd);
void handle_sigchld(int sig);
int init_server();

#endif // MAIN_H