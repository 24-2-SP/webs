#ifndef UTILS_H
#define UTILS_H

// 소켓을 비차단(non-blocking) 모드로 설정
void set_non_blocking(int fd);

// MIME 타입 결정
const char *type(const char *fname);

#endif