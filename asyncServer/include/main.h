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
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdatomic.h>

#define CHUNK_SIZE 65536


// 서버 설정
#define PORT 12345
#define MAX_EVENTS 1024
#define BUFFER_SIZE 512
extern atomic_int active_connections;

// 함수 선언
void handle_connection(int sfd, int epoll_fd);
void close_connection(int cfd, int epoll_fd);
void handle_client_request(int cfd, int epoll_fd);
void handle_req(int cfd, const char *buf);
void handle_get(int cfd, const char *fname);
void response(int cfd, int status, const char *statusM, const char *types, const char *body);
const char *type(const char *fname);
void set_non_blocking(int fd);
void handle_sigchld(int sig);
int init_server();
void handle_head(int cfd, const char *fname);
ssize_t send_data(int cfd, const char *data, size_t length);

#endif // MAIN_H
