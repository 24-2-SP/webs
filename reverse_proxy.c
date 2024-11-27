#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 1024

void handle_request(int client_sock, const char *target_ip, int target_port);
void forward_request(int client_sock, int server_sock);
void send_response(int client_sock, const char *response);

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int port = 1111;  // 사용할 포트

    // 서버 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // 서버 바인딩
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // 서버 리슨
    if (listen(server_sock, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);

    while (1) {
        // 클라이언트 요청 처리
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        // 클라이언트로부터 요청을 받아 처리
        handle_request(client_sock, "10.198.138.213", 2222);  // swist2의 IP와 포트로 요청 전달
        close(client_sock);
    }

    close(server_sock);
    return 0;
}

void handle_request(int client_sock, const char *target_ip, int target_port) {
    int server_sock;
    struct sockaddr_in target_addr;
    char buffer[MAX_BUFFER_SIZE];

    // 서버 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        return;
    }

    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(target_port);
    if (inet_pton(AF_INET, target_ip, &target_addr.sin_addr) <= 0) {
        perror("Invalid target IP");
        close(server_sock);
        return;
    }

    // 서버에 연결
    if (connect(server_sock, (struct sockaddr *)&target_addr, sizeof(target_addr)) < 0) {
        perror("Connection to target server failed");
        close(server_sock);
        return;
    }

    // 요청을 서버로 전달
    forward_request(client_sock, server_sock);

    // 서버로부터 응답을 받아 클라이언트에 전달
    ssize_t total_bytes = 0;
    ssize_t bytes_read;
    while ((bytes_read = read(server_sock, buffer + total_bytes, sizeof(buffer) - total_bytes)) > 0) {
        total_bytes += bytes_read;
    }
    send_response(client_sock, buffer);

    close(server_sock);
}

void forward_request(int client_sock, int server_sock) {
    char buffer[MAX_BUFFER_SIZE];
    ssize_t bytes_read;

    // 클라이언트로부터 데이터를 읽어 서버로 전달
    bytes_read = read(client_sock, buffer, sizeof(buffer));
    if (bytes_read > 0) {
        ssize_t bytes_written = write(server_sock, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror("Write to target server failed");
        }
    }
}

void send_response(int client_sock, const char *response) {
    // HTTP/1.1 응답으로 설정하고, Content-Length와 Connection 헤더 추가
    char http_response[MAX_BUFFER_SIZE];
    snprintf(http_response, sizeof(http_response),
             "HTTP/1.1 200 OK\r\n"
             "Content-Length: %ld\r\n"
             "Content-Type: text/html\r\n"
             "Connection: close\r\n"
             "\r\n", strlen(response));
    strncat(http_response, response, sizeof(http_response) - strlen(http_response) - 1);
    write(client_sock, http_response, strlen(http_response));
}

