#include "spi.h"
#include "tcp.h"

#include "esp_log.h"
#include <freertos/queue.h>
#include <sys/socket.h>

extern QueueHandle_t request_queue;

void spi_request_sender() {
    request_queue_item item;

    while (1) {
        xQueueReceive(request_queue, &item, portMAX_DELAY);
    }
}

void spi_response_receiver(void *pvParameters) {}