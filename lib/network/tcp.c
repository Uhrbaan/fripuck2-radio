#include "tcp.h"
#include "network_config.h"

#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <sys/socket.h>

#define RECEIVE_EVENT_BIT (1UL << 0UL)  // Socket has received data (or is ready to receive)
#define TRANSMIT_EVENT_BIT (1UL << 1UL) // Outgoing queue has new data to send
#define DISCONNECT_BIT (1UL << 2UL)     // Socket has disconnected (optional but recommended)

QueueHandle_t tcp_transmit_queue;
extern QueueHandle_t uart_request_queue; ///< Holds data sent from a remote client.

EventGroupHandle_t tcp_event_group;

void tcp_handler(const int socket) {
    request_queue_item item;
    fd_set read_fd;

    while (1) {
        FD_ZERO(&read_fd);
        FD_SET(socket, &read_fd);

        if (select(socket + 1, &read_fd, NULL, NULL, NULL) < 0) {
            ESP_LOGE("TCP HANDLER", "Failed to create select.");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(socket, &read_fd)) {
            int bytes = recv(socket, item.buffer, sizeof(item.buffer), 0);
            item.size = bytes;
            xQueueSendToBack(uart_request_queue, &item, portMAX_DELAY);
            ESP_LOGI("TCP HANDLER", "Receieved and queued %zu bytes.", bytes);
        }

        if (xQueueReceive(uart_request_queue, &item, 0) == pdTRUE) {
            int bytes = send(socket, item.buffer, sizeof(item.buffer), 0);
            ESP_LOGI("TCP HANDLER", "Dequeued and sent %zu bytes.", bytes);
        }
    }
}

static void net_requests_receiver(const int socket) {
    int length;
    int offset = 0;
    request_queue_item item;

    ESP_LOGI("NET REQUEST RECEIVER", "Starting net_requests_receiver.");
    do {
        length = recv(socket, &item.buffer[offset], sizeof(item), 0);
        offset += length;
        if (length < 0) {
            ESP_LOGE("NET REQUEST RECEIVER", "Error occured during `recv`: errno %d", errno);
            offset = 0;
            continue;
        } else if (length == 0) {
            ESP_LOGE("NET REQUEST RECEIVER", "Connection lost.");
            offset = 0;
            continue;
        } else if (length < 512) { // If less than 512, then nothing more comming: we can send the data to
            item.size = offset;
            offset = 0;
            xQueueSendToBack(uart_request_queue, &item, portMAX_DELAY); // wait if full
            ESP_LOGI("NET REQUEST RECEIVER", "Send data %.500s of length %d.\nThe queue is currently %d items long.",
                     item.buffer, item.size, uxQueueMessagesWaiting(uart_request_queue));
        }

        if (xQueueReceive(uart_request_queue, &item, portMAX_DELAY) == pdTRUE) {
            continue;
        }

    } while (length > 0);
}

static void tcp_client_handler(const int sock) {
    int len;
    char rx_buffer[128];

    do {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);

        if (len < 0) {
            ESP_LOGE("tcp_client_handler", "Error occurred during recv: errno %d", errno);
        } else if (len == 0) {
            ESP_LOGI("tcp_client_handler", "Connection closed by client");
        } else {
            rx_buffer[len] = 0; // Null-terminate whatever is received and print it
            ESP_LOGI("tcp_client_handler", "Received %d bytes: %s", len, rx_buffer);

            // Send the same data back (ECHO)
            int written = send(sock, rx_buffer, len, 0);

            if (written < 0) {
                ESP_LOGE("tcp_client_handler", "Error occurred during send: errno %d", errno);
            } else {
                ESP_LOGI("tcp_client_handler", "Sent %d bytes back.", written);
            }
        }
    } while (len > 0);

    // Clean up the client socket after the loop (connection closed or error)
    shutdown(sock, 0);
    close(sock);
}

int tcp_init(void) {
    tcp_transmit_queue = xQueueCreate(5, sizeof(request_queue_item));
    return ESP_OK;
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
    // TODO: move initialization to a tcp_init function.
    char addr_str[128];
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;

    struct sockaddr_storage dest_addr;

    if (addr_family != AF_INET) {
        ESP_LOGE("tcp_server", "Currently, only IPv4 is supported.");
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
        ESP_LOGE("tcp_server", "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    ESP_LOGI("tcp_server", "Created tcp socket.");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE("tcp_server", "Socket unable to bind: errno %d", errno);
        ESP_LOGE("tcp_server", "IPPROTO: %d", addr_family);
        close(listen_sock);
        vTaskDelete(NULL);
    }
    ESP_LOGI("tcp_server", "Socket bound on port %d", tcp_port);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE("tcp_server", "Error occurred during listen: errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
    }

    while (1) {
        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE("tcp_server", "Unable to accept connection: errno %d", errno);
            break;
        }

        if (source_addr.ss_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }

        ESP_LOGI("tcp_server", "Socket accepted ip address: %s", addr_str);

        // Main loop
        net_requests_receiver(sock);

        shutdown(sock, 0);
        close(sock);
    }

    close(listen_sock);
    vTaskDelete(NULL);
}