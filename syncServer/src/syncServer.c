//동기 http server

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
void response (int cfd, int status, const char *statusM, const char *types, const char *body);
void handle_req(int cfd);
const char *type (const char *fname);
void handle_get(int cfd, const char *fname);

//http 응답 
void response (int cfd, int status, const char *statusM, const char *types, const char *body){
    char buf[4096];
    snprintf(buf, sizeof(buf),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "\r\n"
             "%s",
             status, statusM, types, strlen(body), body);
    write(cfd,buf,strlen(buf));
}


// 클라이언트 요청 처리 : 요청 메소드 매핑
//요청 메소드 : GET, DELETE, POST, PUT, OPTIONS
void handle_req(int cfd)
{
    char buf[4096];
    int bytes = read(cfd, buf, sizeof(buf) - 1);

    if (bytes <= 0)
    {
        perror("read failed");
        close(cfd);
        return;
    }
    buf[bytes] = '\0';

    // 요청 파싱
    char method[16], path[256], protocol[16];
    sscanf(buf, "%s %s %s", method, path, protocol);

    // 요청 메소드에 따른 처리
    if (strcmp(method, "GET") == 0)
    {
        handle_get(cfd, path+1);
    }
    // else if (strcmp(method, "POST") == 0)
    // {
    //     handle_post(cfd, buf);
    // }
    // else if (strcmp(method, "DELETE") == 0)
    // {
    //     handle_delete(cfd, path);
    // }
    // else if (strcmp(method, "PUT") == 0)
    // {
    //     handle_put(cfd, path, buf);
    // }
    // else if (strcmp(method, "OPTIONS") == 0)
    // {
    //     handle_options(cfd);
    // }
    else
    {
        const char *body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
        response(cfd, 405, "Method Not Allowed", "text/html", body);
    }
}

const char *type (const char *fname){
    // 파일 확장자 추출
    const char *ext = strrchr(fname, '.'); // 파일 이름에서 마지막 '.' 위치 찾기
    if (!ext || ext == fname) return "application/octet-stream"; // 확장자가 없거나 파일 이름만 있는 경우 기본값 반환

    // 확장자에 따른 MIME 타입 매핑
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".txt") == 0) return "text/plain";

    // 알 수 없는 확장자는 기본값 반환
    return "application/octet-stream";
}

// GET 요청 처리
void handle_get(int cfd, const char *fname)
{
    FILE *fp = fopen(fname, "rb"); // 바이너리 파일로 오픈
    
    if (!fp)
    {
        response(cfd, 404, "Not Found", "text/plain", "File not found");
        return ;
    }

    // 파일 크기 계산
    fseek(fp, 0, SEEK_END); // 파일 포인터를 파일 끝으로 이동
    long file_size = ftell(fp); // 현재 파일 위치(파일의 크기)를 가져옴
    fseek(fp, 0, SEEK_SET); // 파일 포인터를 처음으로 이동

    char *buf = (char *)malloc(file_size);
    if (buf) {
        fread(buf, 1, file_size, fp); // 파일 내용을 buffer에 저장
    }

    fclose(fp);
    response(cfd, 200, "OK", type(fname),buf);
    free(buf);

}



// SIGCHLD 시그널 핸들러
void handle_sigchld(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        // 종료된 자식 프로세스 회수
    }
}

//서버 초기화 함수
int init(){
    int sfd;
    struct sockaddr_in sin;
    
    //소켓 생성 SOCK_STREAM : tcp 타입
	if((sfd=socket(AF_INET,SOCK_STREAM,0))==-1){
		perror("socket failed");
		exit(1);
	}

    //소켓 옵션 설정
    int opt=1;
    if(setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR, &opt, sizeof(opt))==-1){
        perror("setsockopt failed");
        exit(1);
    }

    //서버 구조체 초기화
    memset((char*)&sin, '\0', sizeof(sin));
    sin.sin_family =AF_INET; //IPv4 사용
	sin.sin_port = htons(12345); //포트 번호 설정
	sin.sin_addr.s_addr=inet_addr("0.0.0.0"); //모든 네트워크에서 연결 허용

    //소켓을 주소와 바인딩
    if(bind(sfd,(struct sockaddr *)&sin, sizeof(sin))==-1){
        perror("bind failed");
        exit(1);
    }
    
    //클라이언트 요청 대기 (최대 1000개)
    if(listen(sfd,1000)==-1){
        perror("listen failed");
        close(sfd);
        exit(1);
    }
    
    printf("server on port 12345 in running\n");
    return sfd;

}



int main (){
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    int sfd=init(); 
    struct sockaddr_in cli;
    socklen_t clen =sizeof(cli);

    while(1){
        // 클라이언트 연결 수락
        int cfd = accept(sfd, (struct sockaddr *)&cli, &clen);
        if (cfd == -1) {
            perror("accept failed");
            continue;
        }
        printf("new client : %d\n", cfd);

        //자식 프로세스 생성 
        pid_t pid =fork();
        if (pid ==0){
            //자식 프로세스
            close(sfd); //서버 소켓 닫기
            handle_req(cfd);
            exit(0);
        } else if (pid > 0) {
            // 부모 프로세스
            close(cfd); //클라이언트 소켓 닫기
        }
    }
    close(sfd);
    return 0;
}