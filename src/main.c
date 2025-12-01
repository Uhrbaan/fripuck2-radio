#include "spi.h"
#include "tcp.h"
#include "wifi.h"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <sys/socket.h>

static const char *TAG = "MAIN";

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(wifi_init());
    ESP_ERROR_CHECK(spi_init());
    ESP_ERROR_CHECK(tcp_init_());

    xTaskCreate(tcp_server, "tcp_server", 4096, (void *)AF_INET, 5, NULL);
    xTaskCreate(spi_request_sender, "spi_request_sender", 4096, NULL, 5, NULL);

    int count = 0;
    while (1) {
        // Log a message every 5 seconds
        ESP_LOGI(TAG, "Alive (%d)", count++);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}