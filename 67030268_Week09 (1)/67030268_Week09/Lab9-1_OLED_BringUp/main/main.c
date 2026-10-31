// ==============================================================================
// Worksheet Lab 9.1: SPI OLED Deconstructed Bringup & Hello World
// Student: นายกร แดงทอง (67030268)
// Course: 03376134 IoT-App-2569 (Semester 2/2568)
// ==============================================================================

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "font5x7.h"

static const char *TAG = "LAB9_1_OLED";

#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5
#define PIN_NUM_DC   2
#define PIN_NUM_RST  4

static spi_device_handle_t s_spi_handle;
static uint8_t s_oled_buffer[1024]; // 128x64 / 8 = 1024 bytes Framebuffer

static esp_err_t oled_spi_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_NUM_DC) | (1ULL << PIN_NUM_RST),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 1024,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000, // 10 MHz
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 7,
    };
    return spi_bus_add_device(SPI2_HOST, &devcfg, &s_spi_handle);
}

static void oled_send_cmd(uint8_t cmd)
{
    gpio_set_level(PIN_NUM_DC, 0);
    spi_transaction_t t = { .length = 8, .tx_buffer = &cmd };
    spi_device_polling_transmit(s_spi_handle, &t);
}

static void oled_flush(void)
{
    gpio_set_level(PIN_NUM_DC, 1);
    spi_transaction_t t = { .length = 1024 * 8, .tx_buffer = s_oled_buffer };
    spi_device_polling_transmit(s_spi_handle, &t);
}

static void oled_clear(void)
{
    memset(s_oled_buffer, 0, sizeof(s_oled_buffer));
}

static void oled_draw_pixel(int x, int y, bool color)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
    int index = x + (y / 8) * 128;
    int bit = y % 8;
    if (color) s_oled_buffer[index] |= (1 << bit);
    else s_oled_buffer[index] &= ~(1 << bit);
}

static void oled_draw_char(int x, int y, char c, bool color)
{
    if (c < 32 || c > 126) c = '?';
    int font_idx = c - 32;
    for (int col = 0; col < 5; col++) {
        uint8_t line = font5x7[font_idx][col];
        for (int row = 0; row < 7; row++) {
            if (line & (1 << row)) oled_draw_pixel(x + col, y + row, color);
            else oled_draw_pixel(x + col, y + row, !color);
        }
    }
    for (int row = 0; row < 7; row++) {
        oled_draw_pixel(x + 5, y + row, !color);
    }
}

static void oled_draw_string(int x, int y, const char *str, bool color)
{
    while (*str) {
        oled_draw_char(x, y, *str, color);
        x += 6;
        if (x + 6 > 128) break;
        str++;
    }
}

static void oled_init_display(void)
{
    // Hardware Reset
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(15));
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(15));

    // SSD1306 Initialization Magic Sequence
    oled_send_cmd(0xAE); // Display OFF
    oled_send_cmd(0x8D); oled_send_cmd(0x14); // Enable Charge Pump 7.5V
    oled_send_cmd(0x20); oled_send_cmd(0x00); // Horizontal Addressing Mode
    oled_send_cmd(0xA1); // Segment Re-map
    oled_send_cmd(0xC8); // COM Output Scan Direction
    oled_send_cmd(0xDA); oled_send_cmd(0x12);
    oled_send_cmd(0x81); oled_send_cmd(0xCF); // Contrast Control
    oled_send_cmd(0xAF); // Display ON
}

void app_main(void)
{
    ESP_LOGI(TAG, "======================================================");
    ESP_LOGI(TAG, "Starting Lab 9.1: SPI OLED Bringup (ID: 67030268)");
    ESP_LOGI(TAG, "Student: นายกร แดงทอง");
    ESP_LOGI(TAG, "======================================================");

    ESP_ERROR_CHECK(oled_spi_init());
    oled_init_display();

    // 1. White screen test pattern (1.5 seconds)
    ESP_LOGI(TAG, "Step 1: Full White Test Pattern (0xFF)");
    memset(s_oled_buffer, 0xFF, sizeof(s_oled_buffer));
    oled_flush();
    vTaskDelay(pdMS_TO_TICKS(1500));

    // 2. Clear & 4 Corners Test Pixels (1.5 seconds)
    ESP_LOGI(TAG, "Step 2: 4 Corners Coordinate Verification");
    oled_clear();
    oled_draw_pixel(0, 0, true);
    oled_draw_pixel(127, 0, true);
    oled_draw_pixel(0, 63, true);
    oled_draw_pixel(127, 63, true);
    oled_flush();
    vTaskDelay(pdMS_TO_TICKS(1500));

    // 3. Render Hello World and Student ID
    ESP_LOGI(TAG, "Step 3: Rendering HELLO WORLD & ID: 67030268");
    oled_clear();
    oled_draw_string(30, 4, "HELLO WORLD", true);
    oled_draw_string(24, 32, "ID: 67030268", true);
    oled_flush();

    // 4. Memory Forensics: Dump Framebuffer
    ESP_LOGI(TAG, "--- Memory Forensics Framebuffer Dump ---");
    printf("[PAGE 0 DUMP]: ");
    for (int i = 0; i < 16; i++) printf("0x%02X ", s_oled_buffer[i]);
    printf("\n");

    printf("[CHAR 'H' DUMP (Cols 30..35)]: ");
    for (int col = 30; col <= 35; col++) printf("0x%02X ", s_oled_buffer[col]);
    printf("\n");
    ESP_LOGI(TAG, "Lab 9.1 Bringup Finished Successfully!");
}
