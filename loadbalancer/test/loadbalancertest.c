#include <stdio.h>
#include "../load_balancer.h"

int main() {
    httpserver servers[] = {
        {"127.0.0.1", 9001, 10},
        {"127.0.0.1", 9002, 2}
    };

    int bigger_weight_server_count = 0;
    int smaller_weight_server_count = 0;

    init_http_servers(servers, 2);

    for (int i = 0; i < 10; i++) {
        httpserver server = round_robin();
        printf("round robin: %s:%d\n", server.ip, server.port);
    }

    for (int i = 0; i < 10; i++) {
        httpserver server = weighted_round_robin();
        if (server.weight == 10) {
            bigger_weight_server_count++;
        } else {
            smaller_weight_server_count++;
        }
        printf("weighted round robin: %s:%d (weight : %d)\n", server.ip, server.port, server.weight);
    }

    printf("bigger: %d times requested\n", bigger_weight_server_count);
    printf("smaller: %d times requested\n", smaller_weight_server_count);

    return 0;
}
