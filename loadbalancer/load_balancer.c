#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "load_balancer.h"

#define MAX_HTTP_SERVERS 2

static httpserver http_servers[MAX_HTTP_SERVERS];
static int http_server_count = 0;
static int current_index = 0;

void init_http_servers(httpserver servers[], int count) {
    if (count > MAX_HTTP_SERVERS) {
        fprintf(stderr, "Error: 최대 %d개의 서버만 사용 가능합니다.\n", MAX_HTTP_SERVERS);
        return;
    }
    
    for (int i = 0; i < count; i++) {
        strcpy(http_servers[i].ip, servers[i].ip);
        http_servers[i].port = servers[i].port;
        http_servers[i].weight = servers[i].weight;
        http_servers[i].current_weight = 0;
    }

    http_server_count = count;
    current_index = 0;
}

httpserver round_robin() {
    if(http_server_count == 0) {
        fprintf(stderr, "사용 가능한 http 서버가 없습니다.\n");
        httpserver empty = {"", 0};
        return empty;
    }

    httpserver server = http_servers[current_index];
    current_index = (current_index + 1) % http_server_count;
    return server;
}

httpserver weighted_round_robin() {
    if(http_server_count == 0) {
        fprintf(stderr, "사용 가능한 http 서버가 없습니다.\n");
        httpserver empty = {"", 0};
        return empty;
    }

    int total_weight = 0;
    for (int i = 0; i < http_server_count; i++) {
        total_weight += http_servers[i].weight;
    }

    int random = rand() % total_weight;
    int weight_sum = 0;
    for (int i = 0; i < http_server_count; i++) {
        weight_sum += http_servers[i].weight;
        if (random < weight_sum) {
            return http_servers[i];
        }
    }

    httpserver empty = {"", 0};
    return empty;
}
