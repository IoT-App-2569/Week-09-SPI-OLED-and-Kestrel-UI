#include <stdio.h>
#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"

#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS    5
#define PIN_NUM_DC    2
#define PIN_NUM_RST   4

static spi_device_handle_t spi;

void oled_cmd(uint8_t cmd) {
    gpio_set_level(PIN_NUM_DC, 0); // Command mode
    spi_transaction_t t = { .length = 8, .tx_buffer = &cmd };
    spi_device_polling_transmit(spi, &t);
}

void app_main(void) {
    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_NUM_RST, 1);

    spi_bus_config_t buscfg = {
        .miso_io_num = -1, .mosi_io_num = PIN_NUM_MOSI, .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1, .quadhd_io_num = -1
    };
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000, // 10 MHz
        .mode = 0, .spics_io_num = PIN_NUM_CS, .queue_size = 7
    };
    spi_bus_add_device(SPI2_HOST, &devcfg, &spi);

    // Initialization sequence
    oled_cmd(0xAE); // Display OFF
    oled_cmd(0x8D); oled_cmd(0x14); // Enable Charge Pump
    oled_cmd(0xAF); // Display ON
    printf("OLED SSD1306 Initialized successfully via 4-Wire SPI!\n");
}
