#ifndef TCP_H
#define TCP_H

typedef struct {
    size_t size;
    uint8_t buffer[512];
} request_queue_item;

void tcp_server(void *pvParameters);

#endif