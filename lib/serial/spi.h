#ifndef SPI_H
#define SPI_H

#include "esp_err.h"

esp_err_t spi_init(void);
void spi_request_sender(void *pvParameters);

#endif