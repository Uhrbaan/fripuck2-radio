#include "network_config.h"

#include "esp_log.h"
#include <sys/socket.h>

static const char TAG[] = "TCP";

static void tcp_client_handler(const int sock) {
    int len;
    char rx_buffer[128];

    do {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);

        if (len < 0) {
            ESP_LOGE(TAG, "Error occurred during recv: errno %d", errno);
        } else if (len == 0) {
            ESP_LOGI(TAG, "Connection closed by client");
        } else {
            rx_buffer[len] = 0; // Null-terminate whatever is received and print it
            ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);

            // Send the same data back (ECHO)
            int written = send(sock, rx_buffer, len, 0);

            if (written < 0) {
                ESP_LOGE(TAG, "Error occurred during send: errno %d", errno);
            } else {
                ESP_LOGI(TAG, "Sent %d bytes back.", written);
            }
        }
    } while (len > 0);

    // Clean up the client socket after the loop (connection closed or error)
    shutdown(sock, 0);
    close(sock);
}

/**
 * @brief Create a TCP server task
 *
 * This function should be run in a FreeRTOS task.
 * Currently, only IPv4 is supported, so give AF_INET as a parameter in your task.
 *
 * @param pvParameters
 */
void tcp_server(void *pvParameters) {
    // Initialization
    char addr_str[128];
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;

    struct sockaddr_storage dest_addr;

    if (addr_family != AF_INET) {
        ESP_LOGE(TAG, "Currently, only IPv4 is supported.");
        return;
    }

    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(tcp_port);
    ip_protocol = IPPROTO_IP;

    // Create listening socket
    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    ESP_LOGI(TAG, "Created tcp socket.");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        close(listen_sock);
        vTaskDelete(NULL);
    }
    ESP_LOGI(TAG, "Socket bound on port %d", tcp_port);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
    }

    while (1) {
        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        if (source_addr.ss_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }

        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        // Main loop
        tcp_client_handler(sock);

        shutdown(sock, 0);
        close(sock);
    }

    close(listen_sock);
    vTaskDelete(NULL);
}