#include "tcp.h"
#include "wifi.h"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <sys/socket.h>

// Define a tag for logging purposes
static const char *TAG = "MAIN";

// The entry point for all ESP-IDF applications
void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());

    // Function blocking until connected or failed.
    wifi_init();

    // Start the TCP server
    xTaskCreate(tcp_server, "tcp_server", 4096, (void *)AF_INET, 5, NULL);

    // Simple loop to prove the main task is running and FreeRTOS is operational
    int count = 0;
    while (1) {
        // Log a message every 5 seconds
        ESP_LOGI(TAG, "Alive (%d)", count++);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}