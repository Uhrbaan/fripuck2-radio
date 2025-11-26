#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <stdio.h>

// Define a tag for logging purposes
static const char *TAG = "APP_MAIN";

// The entry point for all ESP-IDF applications
void app_main(void) {
    ESP_LOGI(TAG, "===================================");
    ESP_LOGI(TAG, "Test Application Started Successfully!");
    ESP_LOGI(TAG, "Chip model: %s",
             CONFIG_IDF_TARGET); // Logs the chip model configured in sdkconfig
    ESP_LOGI(TAG, "===================================");

    // Simple loop to prove the main task is running and FreeRTOS is operational
    int count = 0;
    while (1) {
        // Log a message every 5 seconds
        ESP_LOGI(TAG, "Hello World! Loop count: %d", count++);
        printf("This, is a test. If it works, I'm happy but also confused.\r\n");
        fflush(stdout);

        // Delay for 5000 milliseconds (5 seconds)
        // The time is specified in "ticks." pdMS_TO_TICKS macro converts ms to
        // ticks.
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}