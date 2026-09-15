#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../../Assests/font5x7.h"

#define OLED_SCK GPIO_NUM_18
#define OLED_MOSI GPIO_NUM_23
#define OLED_RES GPIO_NUM_4
#define OLED_DC GPIO_NUM_2
#define OLED_CS GPIO_NUM_5
#define OLED_W 128
#define OLED_H 64
#define POT_CHANNEL ADC_CHANNEL_6 /* ADC1 GPIO34 */

static const char *TAG = "iot_loop";
static spi_device_handle_t spi;
static uint8_t frame[OLED_W * OLED_H / 8];

static esp_err_t send_cmd(uint8_t command) {
    spi_transaction_t t = {.length = 8, .tx_buffer = &command};
    gpio_set_level(OLED_DC, 0);
    return spi_device_polling_transmit(spi, &t);
}

static esp_err_t send_data(const uint8_t *data, size_t length) {
    spi_transaction_t t = {.length = length * 8, .tx_buffer = data};
    gpio_set_level(OLED_DC, 1);
    return spi_device_polling_transmit(spi, &t);
}

static void oled_init(void) {
    gpio_config_t pins = {
        .pin_bit_mask = (1ULL << OLED_DC) | (1ULL << OLED_RES),
        .mode = GPIO_MODE_OUTPUT, .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE, .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&pins));
    spi_bus_config_t bus = {.miso_io_num = -1, .mosi_io_num = OLED_MOSI,
        .sclk_io_num = OLED_SCK, .quadwp_io_num = -1, .quadhd_io_num = -1,
        .max_transfer_sz = sizeof(frame) + 16};
    spi_device_interface_config_t device = {.clock_speed_hz = 10 * 1000 * 1000,
        .mode = 0, .spics_io_num = OLED_CS, .queue_size = 7};
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &device, &spi));
    gpio_set_level(OLED_RES, 0); vTaskDelay(pdMS_TO_TICKS(15));
    gpio_set_level(OLED_RES, 1); vTaskDelay(pdMS_TO_TICKS(15));
    const uint8_t init[] = {
        0xAE,             // Display off
        0x8D, 0x14,       // Enable charge pump
        0x20, 0x00,       // Horizontal memory addressing
        0xA1,             // Segment re-map: flip horizontally
        0xC8,             // COM scan direction: flip vertically
        0xAF              // Display on
    };
    for (size_t i = 0; i < sizeof(init); i++) ESP_ERROR_CHECK(send_cmd(init[i]));
}

static void pixel(int x, int y, bool on) {
    if (x < 0 || x >= OLED_W || y < 0 || y >= OLED_H) return;
    int index = x + (y / 8) * OLED_W;
    uint8_t bit = 1U << (y % 8);
    if (on) frame[index] |= bit; else frame[index] &= (uint8_t)~bit;
}

static void text(int x, int y, const char *value) {
    while (*value && x + 5 < OLED_W) {
        char c = (*value < 32 || *value > 126) ? '?' : *value;
        for (int col = 0; col < 5; col++)
            for (int row = 0; row < 7; row++)
                pixel(x + col, y + row, font5x7[c - 32][col] & (1U << row));
        x += 6; value++;
    }
}

static void flush(void) {
    const uint8_t addressing[] = {0x21, 0x00, 0x7F, 0x22, 0x00, 0x07};
    for (size_t i = 0; i < sizeof(addressing); i++) ESP_ERROR_CHECK(send_cmd(addressing[i]));
    ESP_ERROR_CHECK(send_data(frame, sizeof(frame)));
}

static adc_oneshot_unit_handle_t adc_init(void) {
    adc_oneshot_unit_handle_t handle;
    adc_oneshot_unit_init_cfg_t unit = {.unit_id = ADC_UNIT_1};
    adc_oneshot_chan_cfg_t channel = {.atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT};
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit, &handle));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(handle, POT_CHANNEL, &channel));
    return handle;
}

static bool serial_line(char *out, size_t capacity) {
    static size_t used;
    uint8_t c;
    while (uart_read_bytes(UART_NUM_0, &c, 1, 0) == 1) {
        if (c == '\r') continue;
        if (c == '\n') { out[used] = '\0'; used = 0; return true; }
        if (used < capacity - 1) out[used++] = (char)c; else used = 0;
    }
    return false;
}

static void draw_ui(int percent, int raw, const char *message) {
    char raw_text[16], percent_text[8];
    percent = percent < 0 ? 0 : (percent > 100 ? 100 : percent);
    snprintf(raw_text, sizeof(raw_text), "RAW:%4d", raw);
    snprintf(percent_text, sizeof(percent_text), "%3d%%", percent);
    memset(frame, 0, sizeof(frame));
    text(2, 0, "ESP32 OK"); text(2, 10, raw_text); text(90, 10, percent_text);
    for (int x = 2; x < 126; x++) { pixel(x, 23, true); pixel(x, 34, true); }
    for (int y = 23; y < 35; y++) { pixel(2, y, true); pixel(125, y, true); }
    for (int x = 0; x < percent * 120 / 100; x++)
        for (int y = 0; y < 7; y++) pixel(4 + x, 25 + y, true);
    text(2, 45, message); flush();
}

void app_main(void) {
    oled_init();
    adc_oneshot_unit_handle_t adc = adc_init();
    int raw = 0, percent = 0;
    char message[21] = "SYSTEM READY", rx[64];
    while (true) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc, POT_CHANNEL, &raw));
        printf("ADC:%d\n", raw); fflush(stdout);
        while (serial_line(rx, sizeof(rx))) {
            char received[21] = {0};
            if (sscanf(rx, "%d,%20s", &percent, received) == 2) {
                for (char *p = received; *p; p++) if (*p == '_') *p = ' ';
                strncpy(message, received, sizeof(message) - 1);
                message[sizeof(message) - 1] = '\0';
            }
        }
        draw_ui(percent, raw, message);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
