#ifndef LOAD_BALANCER_H
#define LOAD_BALANCER_H

#include <stdio.h>
#include <string.h>

typedef struct httpserver {
    char ip[16];
    int port;
    int weight;
    int active_connections;
} httpserver;

void init_http_servers(httpserver servers[], int count);

httpserver round_robin();
httpserver weighted_round_robin();
httpserver least_connection();

#endif
