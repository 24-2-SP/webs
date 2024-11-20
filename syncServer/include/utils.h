#ifndef UTILS_H
#define UTILS_H

// MIME 타입 결정 함수: 파일 이름을 기반으로 MIME 타입 반환
const char *type(const char *fname);
// SIGCHLD 시그널 핸들러: 종료된 자식 프로세스 회수
void handle_sigchld(int sig);


#endif // UTILS_H