#ifndef TCP_H
#define TCP_H

#include <stddef.h>
#include <stdint.h>

typedef struct request_queue_item {
    size_t size;
    uint8_t buffer[512];
} request_queue_item;

void tcp_server(void *pvParameters);

#endif