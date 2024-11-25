# 컴파일러 및 옵션 설정
CC = gcc
CFLAGS = -Wall -g -Iinclude

# 소스 파일 정의
SRC = main.c src/asyncServer.c src/http.c src/utils.c
OBJ = $(SRC:.c=.o)

# 기본 타겟
all: asyncserver

# syncserver 실행 파일 생성
asyncserver: $(OBJ)
	$(CC) $(CFLAGS) -o asyncserver $(OBJ)

# .o 파일 생성 규칙
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 클린업
clean:
	rm -f asyncserver $(OBJ)