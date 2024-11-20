#ifndef HTTP_H
#define HTTP_H

#define BUFFER_SIZE 4096

// 클라이언트 요청 처리 함수
void handle_client_request(int cfd, int epoll_fd);

// HTTP 요청을 분석하고 처리
void handle_req(int cfd, const char *buf);

// GET 요청 처리 함수
void handle_get(int cfd, const char *fname);

#endif