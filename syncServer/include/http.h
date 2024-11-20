#ifndef HTTP_H
#define HTTP_H

#define BUFFER_SIZE 4096


// HTTP 요청을 분석하고 처리
void handle_req(int cfd);

// GET 요청 처리 함수
void handle_get(int cfd, const char *fname);

// HTTP 응답 전송 함수: 상태 코드, 상태 메시지, 컨텐츠 타입, 본문 데이터를 클라이언트로 전송
void response(int cfd, int status, const char *statusM, const char *types, const char *body);

#endif