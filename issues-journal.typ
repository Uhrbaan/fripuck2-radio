== 2025.12.01
So, found out the problem why the wifi would not want to connect to the wifi.
according to https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/wifi.html#esp-wifi-reason-code
and the logs, the problem is
```
NO_AP_FOUND_AUTHMODE (211):
Espressif-specific Wi-Fi reason code: NO_AP_FOUND_IN_AUTHMODE_THRESHOLD will be reported if an AP that fit identifying criteria (e.g. ssid) is found but the authmode threhsold set in the wifi_config_t is not met.
```

This suggest that the mode I put in is not supported by the `robotics_lab` router.
So I changed the `.threshold.authmode` setting to use WP2 instead of mixed wp2/wp3, which seemed to have fixed the issue.

Now, I am facing another issue with the queue that throws an error.
Here is the backtrace:
```
❯ pio pkg exec --package "espressif/toolchain-xtensa-esp32s3" -- xtensa-esp32s3-elf-addr2line -pfiaC -e .pio/build/esp32dev/firmware.elf 0x40082e7a:0x3ffcac30 0x4008b731:0x3ffcac60 0x40093f6a:0x3ffcac90 0x4008c1e1:0x3ffcadc0 0x400d23b7:0x3ffcae00
Using toolchain-xtensa-esp32s3@12.2.0+20230208 package
0x40082e7a: panic_abort at /home/luclement/.platformio/packages/framework-espidf/components/esp_system/panic.c:469
0x4008b731: esp_system_abort at /home/luclement/.platformio/packages/framework-espidf/components/esp_system/port/esp_system_chip.c:87
0x40093f6a: __assert_func at /home/luclement/.platformio/packages/framework-espidf/components/newlib/src/assert.c:80
0x4008c1e1: xQueueReceive at /home/luclement/.platformio/packages/framework-espidf/components/freertos/FreeRTOS-Kernel/queue.c:1535 (discriminator 2)
0x400d23b7: spi_request_sender at /home/luclement/Projets/fripuck2-radio/lib/serial/spi.c:86
```

== 2025.12.03
So, I changed so that the tcp packets go over UART. Now, sending packets work.

You can try an asercom command. The following commands:
```sh
printf "%b" '\xb4\x09\x01' | nc 192.168.2.147 1000
printf "%b" '\xb4\x09\x00' | nc 192.168.2.147 1000
```

Respectively enable and disable the front LED.
Note That you need to replace the IP address with the correct one.
