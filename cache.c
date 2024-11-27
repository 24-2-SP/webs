#include "cache.h"
#include <stdio.h>
#include <string.h>

#define CACHE_SIZE 5

typedef struct {
    char url[256];
    char data[MAX_BUFFER_SIZE];
} CacheEntry;

static CacheEntry cache[CACHE_SIZE];
static int cache_index = 0;

void cache_init() {
    memset(cache, 0, sizeof(cache));
}

int cache_lookup(const char *url, char *data) {
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (strcmp(cache[i].url, url) == 0) {
            strcpy(data, cache[i].data);
            return 1;  // 캐시에서 데이터를 찾은 경우
        }
    }
    return 0;  // 캐시에서 데이터 미발견
}

void cache_insert(const char *url, const char *data) {
    strcpy(cache[cache_index].url, url);
    strncpy(cache[cache_index].data, data, MAX_BUFFER_SIZE);

    cache_index = (cache_index + 1) % CACHE_SIZE;
}

