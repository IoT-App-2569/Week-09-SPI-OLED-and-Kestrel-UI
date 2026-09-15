#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "font5x7.h"


// ============================================================
// OLED / ADC / UART PIN CONFIGURATION
// ============================================================

// OLED SPI pins
#define OLED_PIN_SCK   (GPIO_NUM_18)   // D0 / SPI Clock
#define OLED_PIN_MOSI  (GPIO_NUM_23)   // D1 / SPI MOSI Data
#define OLED_PIN_RES   (GPIO_NUM_4)    // RES / Hardware Reset
#define OLED_PIN_DC    (GPIO_NUM_2)    // DC: 0 = Command, 1 = Data
#define OLED_PIN_CS    (GPIO_NUM_5)    // CS: Active LOW

// Potentiometer
// GPIO34 = ADC1_CHANNEL_6 on classic ESP32
#define POT_ADC_CHANNEL (ADC_CHANNEL_6)

// Serial bridge
#define SERIAL_UART (UART_NUM_0)


// ============================================================
// GLOBAL VARIABLES
// ============================================================

static spi_device_handle_t s_spi_handle = NULL;

// SSD1306 128x64 framebuffer
// 128 x 64 / 8 = 1024 bytes
static uint8_t s_oled_buffer[1024];


// ============================================================
// OLED SPI INITIALIZATION
// ============================================================

esp_err_t oled_spi_init(void)
{
    // --------------------------------------------------------
    // Configure DC and RES GPIO
    // --------------------------------------------------------

    gpio_config_t io_conf = {
        .pin_bit_mask =
            (1ULL << OLED_PIN_DC) |
            (1ULL << OLED_PIN_RES),

        .mode = GPIO_MODE_OUTPUT,

        .pull_up_en = GPIO_PULLUP_ENABLE,

        .pull_down_en = GPIO_PULLDOWN_DISABLE,

        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));


    // --------------------------------------------------------
    // Configure SPI bus
    // Write-only OLED, so MISO is not required
    // --------------------------------------------------------

    spi_bus_config_t buscfg = {
        .miso_io_num = -1,

        .mosi_io_num = OLED_PIN_MOSI,

        .sclk_io_num = OLED_PIN_SCK,

        .quadwp_io_num = -1,

        .quadhd_io_num = -1,

        .max_transfer_sz = 1024 + 16,
    };


    // SPI2_HOST = VSPI on classic ESP32
    esp_err_t ret = spi_bus_initialize(
        SPI2_HOST,
        &buscfg,
        SPI_DMA_CH_AUTO
    );

    if (ret != ESP_OK) {
        return ret;
    }


    // --------------------------------------------------------
    // Configure OLED SPI device
    // --------------------------------------------------------

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000,   // 10 MHz

        .mode = 0,                             // SPI Mode 0

        .spics_io_num = OLED_PIN_CS,           // GPIO 5

        .queue_size = 7,
    };


    return spi_bus_add_device(
        SPI2_HOST,
        &devcfg,
        &s_spi_handle
    );
}


// ============================================================
// OLED SEND COMMAND
// ============================================================

void oled_send_cmd(uint8_t cmd)
{
    // DC LOW = command
    gpio_set_level(OLED_PIN_DC, 0);

    spi_transaction_t t;

    memset(&t, 0, sizeof(t));

    // 1 byte = 8 bits
    t.length = 8;

    t.tx_buffer = &cmd;

    ESP_ERROR_CHECK(
        spi_device_polling_transmit(
            s_spi_handle,
            &t
        )
    );
}


// ============================================================
// OLED SEND DATA
// ============================================================

void oled_send_data(const uint8_t *data, size_t len)
{
    if (len == 0) {
        return;
    }

    // DC HIGH = display data
    gpio_set_level(OLED_PIN_DC, 1);

    spi_transaction_t t;

    memset(&t, 0, sizeof(t));

    // length is in bits
    t.length = len * 8;

    t.tx_buffer = data;

    ESP_ERROR_CHECK(
        spi_device_polling_transmit(
            s_spi_handle,
            &t
        )
    );
}


// ============================================================
// OLED CLEAR FRAMEBUFFER
// ============================================================

void oled_clear(void)
{
    memset(
        s_oled_buffer,
        0x00,
        sizeof(s_oled_buffer)
    );
}


// ============================================================
// OLED DRAW PIXEL
// ============================================================

void oled_draw_pixel(
    int x,
    int y,
    bool color
)
{
    // Check screen boundary
    if (x < 0 || x >= 128 ||
        y < 0 || y >= 64) {

        return;
    }


    // SSD1306 framebuffer layout:
    //
    // byte index =
    // x + (page * 128)
    //
    // page = y / 8

    int byte_index =
        x + (y / 8) * 128;

    int bit_offset =
        y % 8;


    if (color) {

        // Turn pixel ON
        s_oled_buffer[byte_index] |=
            (1 << bit_offset);

    } else {

        // Turn pixel OFF
        s_oled_buffer[byte_index] &=
            ~(1 << bit_offset);
    }
}


// ============================================================
// OLED FLUSH FRAMEBUFFER
// ============================================================

void oled_flush(void)
{
    // Set Column Address
    oled_send_cmd(0x21);

    // Start column
    oled_send_cmd(0x00);

    // End column
    oled_send_cmd(0x7F);


    // Set Page Address
    oled_send_cmd(0x22);

    // Start page
    oled_send_cmd(0x00);

    // End page
    oled_send_cmd(0x07);


    // Send entire 1024-byte framebuffer
    oled_send_data(
        s_oled_buffer,
        sizeof(s_oled_buffer)
    );
}


// ============================================================
// OLED DRAW CHARACTER
// ============================================================

void oled_draw_char(
    int x,
    int y,
    char c,
    bool color
)
{
    // Only support ASCII 32-126
    if (c < 32 || c > 126) {
        c = '?';
    }


    int font_idx = c - 32;


    // Each character is 5 columns wide
    for (int col = 0; col < 5; col++) {

        uint8_t line =
            font5x7[font_idx][col];


        // Each character is 7 pixels high
        for (int row = 0; row < 7; row++) {

            bool pixel =
                (line & (1 << row)) != 0;


            if (pixel) {

                oled_draw_pixel(
                    x + col,
                    y + row,
                    color
                );

            } else {

                oled_draw_pixel(
                    x + col,
                    y + row,
                    !color
                );
            }
        }
    }


    // One blank column between characters
    for (int row = 0; row < 7; row++) {

        oled_draw_pixel(
            x + 5,
            y + row,
            !color
        );
    }
}


// ============================================================
// OLED DRAW STRING
// ============================================================

void oled_draw_string(
    int x,
    int y,
    const char *str,
    bool color
)
{
    while (*str) {

        oled_draw_char(
            x,
            y,
            *str,
            color
        );

        // Character width = 6 pixels
        x += 6;


        // Stop if screen is full
        if (x + 6 > 128) {
            break;
        }

        str++;
    }
}


// ============================================================
// ADC INITIALIZATION
// ============================================================

static adc_oneshot_unit_handle_t
init_potentiometer_adc(void)
{
    adc_oneshot_unit_handle_t adc_handle = NULL;


    // --------------------------------------------------------
    // ADC Unit configuration
    // --------------------------------------------------------

    adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = ADC_UNIT_1,
    };


    ESP_ERROR_CHECK(
        adc_oneshot_new_unit(
            &unit_config,
            &adc_handle
        )
    );


    // --------------------------------------------------------
    // ADC Channel configuration
    // --------------------------------------------------------

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,

        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };


    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(
            adc_handle,
            POT_ADC_CHANNEL,
            &channel_config
        )
    );


    return adc_handle;
}


// ============================================================
// UART INITIALIZATION
// ============================================================

static void init_serial_bridge(void)
{
    const uart_config_t uart_config = {

        .baud_rate = 115200,

        .data_bits = UART_DATA_8_BITS,

        .parity = UART_PARITY_DISABLE,

        .stop_bits = UART_STOP_BITS_1,

        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,

        .source_clk = UART_SCLK_DEFAULT,
    };


    // Configure UART
    ESP_ERROR_CHECK(
        uart_param_config(
            SERIAL_UART,
            &uart_config
        )
    );


    // UART0 uses default USB-UART pins
    ESP_ERROR_CHECK(
        uart_set_pin(
            SERIAL_UART,

            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE,

            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE
        )
    );


    // Install UART driver
    ESP_ERROR_CHECK(
        uart_driver_install(
            SERIAL_UART,

            256,    // RX buffer

            256,    // TX buffer

            0,

            NULL,

            0
        )
    );
}


// ============================================================
// READ SERIAL LINE
// ============================================================

static bool read_serial_line(
    char *line,
    size_t line_size
)
{
    static size_t line_length = 0;

    static char line_buffer[64];

    uint8_t byte;


    while (
        uart_read_bytes(
            SERIAL_UART,
            &byte,
            1,
            0
        ) == 1
    ) {

        // ----------------------------------------------------
        // New line
        // ----------------------------------------------------

        if (byte == '\n' ||
            byte == '\r') {

            if (line_length == 0) {
                continue;
            }


            size_t copy_length =
                line_length < line_size - 1
                ? line_length
                : line_size - 1;


            memcpy(
                line,
                line_buffer,
                copy_length
            );


            line[copy_length] = '\0';


            line_length = 0;


            return true;
        }


        // ----------------------------------------------------
        // Store received character
        // ----------------------------------------------------

        if (
            line_length <
            sizeof(line_buffer) - 1
        ) {

            line_buffer[line_length++] =
                (char)byte;
        }
    }


    return false;
}


// ============================================================
// ADC RAW VALUE -> PERCENT
// ============================================================

static int adc_to_percent(int raw_value)
{
    // Calibration values
    const int raw_min = 150;

    const int raw_max = 3950;


    if (raw_value <= raw_min) {
        return 0;
    }


    if (raw_value >= raw_max) {
        return 100;
    }


    return (
        (raw_value - raw_min) * 100
    ) / (raw_max - raw_min);
}


// ============================================================
// PARSE KESTREL COMMAND
// ============================================================
//
// Supported:
//
// SET:50:CALIBRATED OK
//
// OR
//
// 50,CALIBRATED OK
//
// ============================================================

static bool parse_serial_command(
    const char *line,
    int *percent,
    char *message
)
{
    // Format:
    // SET:50:CALIBRATED OK

    if (
        sscanf(
            line,
            "SET:%d:%31[^\r\n]",
            percent,
            message
        ) == 2
    ) {
        return true;
    }


    // Format:
    // 50,CALIBRATED OK

    if (
        sscanf(
            line,
            "%d,%31[^\r\n]",
            percent,
            message
        ) == 2
    ) {
        return true;
    }


    message[0] = '\0';

    return false;
}


// ============================================================
// RENDER OLED UI
// ============================================================

static void render_multizone_ui(
    int percent,
    int raw_value,
    const char *message
)
{
    char value_line[24];

    char raw_line[24];


    // Clamp percentage
    if (percent < 0) {
        percent = 0;
    }

    if (percent > 100) {
        percent = 100;
    }


    // Create text
    snprintf(
        value_line,
        sizeof(value_line),
        "VALUE: %d%%",
        percent
    );


    snprintf(
        raw_line,
        sizeof(raw_line),
        "RAW: %d",
        raw_value
    );


    // Clear framebuffer
    oled_clear();


    // --------------------------------------------------------
    // Zone 1
    // --------------------------------------------------------

    oled_draw_string(
        0,
        0,
        "ESP32 OK",
        true
    );


    // --------------------------------------------------------
    // Zone 2
    // --------------------------------------------------------

    oled_draw_string(
        0,
        16,
        value_line,
        true
    );


    // --------------------------------------------------------
    // Zone 3
    // --------------------------------------------------------

    oled_draw_string(
        0,
        32,
        raw_line,
        true
    );


    // --------------------------------------------------------
    // Zone 4
    // --------------------------------------------------------

    oled_draw_string(
        0,
        48,
        message,
        true
    );


    // Send framebuffer to OLED
    oled_flush();
}


// ============================================================
// FORENSIC FRAMEBUFFER DUMP
// ============================================================

static void oled_dump_forensic_buffer(void)
{
    const int start_col = 30;

    const int dump_length = 16;


    ESP_LOGI(
        "FORENSIC",
        "=== DUMPING FRAMEBUFFER PAGE 0 (Cols 30-45) ==="
    );


    for (
        int offset = 0;
        offset < dump_length;
        offset++
    ) {

        int col =
            start_col + offset;


        uint8_t byte =
            s_oled_buffer[col];


        printf(
            "Byte[%2d] (Col %2d): 0x%02X "
            "[Binary: %c%c%c%c%c%c%c%c]\n",

            col,
            col,
            byte,

            (byte & 0x80) ? '1' : '0',
            (byte & 0x40) ? '1' : '0',
            (byte & 0x20) ? '1' : '0',
            (byte & 0x10) ? '1' : '0',
            (byte & 0x08) ? '1' : '0',
            (byte & 0x04) ? '1' : '0',
            (byte & 0x02) ? '1' : '0',
            (byte & 0x01) ? '1' : '0'
        );
    }
}


// ============================================================
// FORENSIC FONT DUMP
// ============================================================

static void oled_dump_font_pattern_forensic(char c)
{
    if (c < 32 || c > 126) {
        return;
    }


    int idx = c - 32;


    printf(
        "Forensic reconstruction for '%c' "
        "(ASCII %d)\n",
        c,
        c
    );


    for (int col = 0; col < 5; col++) {

        uint8_t byte =
            font5x7[idx][col];


        printf(
            "Col %d: 0x%02X "
            "[Binary: %c%c%c%c%c%c%c%c]\n",

            col,
            byte,

            (byte & 0x80) ? '1' : '0',
            (byte & 0x40) ? '1' : '0',
            (byte & 0x20) ? '1' : '0',
            (byte & 0x10) ? '1' : '0',
            (byte & 0x08) ? '1' : '0',
            (byte & 0x04) ? '1' : '0',
            (byte & 0x02) ? '1' : '0',
            (byte & 0x01) ? '1' : '0'
        );
    }
}


// ============================================================
// SSD1306 INITIALIZATION
// ============================================================

static void oled_init_display(void)
{
    // Hardware reset
    gpio_set_level(
        OLED_PIN_RES,
        0
    );

    vTaskDelay(
        pdMS_TO_TICKS(15)
    );


    gpio_set_level(
        OLED_PIN_RES,
        1
    );

    vTaskDelay(
        pdMS_TO_TICKS(15)
    );


    // --------------------------------------------------------
    // SSD1306 initialization sequence
    // --------------------------------------------------------

    // Display OFF
    oled_send_cmd(0xAE);


    // Set display clock divide ratio
    oled_send_cmd(0xD5);
    oled_send_cmd(0x80);


    // Set multiplex ratio
    oled_send_cmd(0xA8);
    oled_send_cmd(0x3F);


    // Set display offset
    oled_send_cmd(0xD3);
    oled_send_cmd(0x00);


    // Set start line
    oled_send_cmd(0x40);


    // Enable charge pump
    oled_send_cmd(0x8D);
    oled_send_cmd(0x14);


    // Memory addressing mode
    oled_send_cmd(0x20);

    // Horizontal addressing mode
    oled_send_cmd(0x00);


    // Segment remap
    oled_send_cmd(0xA1);


    // COM output scan direction
    oled_send_cmd(0xC8);


    // COM pins hardware configuration
    oled_send_cmd(0xDA);
    oled_send_cmd(0x12);


    // Contrast
    oled_send_cmd(0x81);
    oled_send_cmd(0x8F);


    // Pre-charge period
    oled_send_cmd(0xD9);
    oled_send_cmd(0xF1);


    // VCOMH deselect level
    oled_send_cmd(0xDB);
    oled_send_cmd(0x40);


    // Entire display ON/OFF follows RAM
    oled_send_cmd(0xA4);


    // Normal display
    oled_send_cmd(0xA6);


    // Display ON
    oled_send_cmd(0xAF);


    vTaskDelay(
        pdMS_TO_TICKS(100)
    );
}


// ============================================================
// OLED TEST SCREEN
// ============================================================

static void oled_test_screen(void)
{
    // --------------------------------------------------------
    // Full white screen
    // --------------------------------------------------------

    uint8_t buffer[128];

    memset(
        buffer,
        0xFF,
        sizeof(buffer)
    );


    oled_send_cmd(0x21);
    oled_send_cmd(0x00);
    oled_send_cmd(0x7F);

    oled_send_cmd(0x22);
    oled_send_cmd(0x00);
    oled_send_cmd(0x07);


    for (int page = 0; page < 8; page++) {

        oled_send_data(
            buffer,
            sizeof(buffer)
        );
    }


    vTaskDelay(
        pdMS_TO_TICKS(1500)
    );


    // --------------------------------------------------------
    // Corner pixel test
    // --------------------------------------------------------

    oled_clear();


    oled_draw_pixel(
        0,
        0,
        true
    );


    oled_draw_pixel(
        127,
        0,
        true
    );


    oled_draw_pixel(
        0,
        63,
        true
    );


    oled_draw_pixel(
        127,
        63,
        true
    );


    oled_flush();


    vTaskDelay(
        pdMS_TO_TICKS(1500)
    );


    // --------------------------------------------------------
    // Text test
    // --------------------------------------------------------

    oled_clear();


    oled_draw_string(
        30,
        2,
        "Hello World",
        true
    );


    oled_draw_string(
        28,
        24,
        "ID: 67030109",
        true
    );


    oled_draw_string(
        28,
        46,
        "ID: 67030085",
        true
    );


    oled_flush();


    vTaskDelay(
        pdMS_TO_TICKS(1000)
    );
}


// ============================================================
// APP MAIN
// ============================================================

void app_main(void)
{
    // ========================================================
    // 1. Initialize OLED SPI
    // ========================================================

    ESP_ERROR_CHECK(
        oled_spi_init()
    );


    // ========================================================
    // 2. Initialize SSD1306
    // ========================================================

    oled_init_display();


    // ========================================================
    // 3. OLED bring-up test
    // ========================================================

    oled_test_screen();


    // ========================================================
    // 4. Forensic debugging
    // ========================================================

    oled_dump_forensic_buffer();

    oled_dump_font_pattern_forensic('H');


    // ========================================================
    // 5. Initialize ADC
    // ========================================================

    adc_oneshot_unit_handle_t adc_handle =
        init_potentiometer_adc();


    // ========================================================
    // 6. Initialize UART
    // ========================================================

    init_serial_bridge();


    // ========================================================
    // 7. Variables
    // ========================================================

    char rx_buffer[64];


    char message[32] =
        "WAITING SERVER";


    int percent = 0;

    int raw_value = 0;


    // ========================================================
    // 8. Initial OLED UI
    // ========================================================

    render_multizone_ui(
        percent,
        raw_value,
        message
    );


    ESP_LOGI(
        "BRIDGE",
        "Two-way serial bridge ready: "
        "ADC GPIO34 -> UART0"
    );


    // ========================================================
    // 9. Main loop
    // ========================================================

    while (1) {

        // ----------------------------------------------------
        // Read potentiometer ADC
        // ----------------------------------------------------

        ESP_ERROR_CHECK(
            adc_oneshot_read(
                adc_handle,
                POT_ADC_CHANNEL,
                &raw_value
            )
        );


        // ----------------------------------------------------
        // Convert ADC -> percentage
        // ----------------------------------------------------

        percent =
            adc_to_percent(raw_value);


        // ----------------------------------------------------
        // Send ADC value to Kestrel
        // ----------------------------------------------------

        printf(
            "ADC:%d\n",
            raw_value
        );


        // ----------------------------------------------------
        // Update OLED
        // ----------------------------------------------------

        render_multizone_ui(
            percent,
            raw_value,
            message
        );


        // ----------------------------------------------------
        // Check incoming Kestrel command
        // ----------------------------------------------------

        if (
            read_serial_line(
                rx_buffer,
                sizeof(rx_buffer)
            )
        ) {

            char received_message[32];

            int received_percent;


            // ------------------------------------------------
            // Parse command
            // ------------------------------------------------

            if (
                parse_serial_command(
                    rx_buffer,
                    &received_percent,
                    received_message
                )
            ) {

                // --------------------------------------------
                // Update message
                // --------------------------------------------

                strncpy(
                    message,
                    received_message,
                    sizeof(message) - 1
                );


                message[
                    sizeof(message) - 1
                ] = '\0';


                // --------------------------------------------
                // Update percentage if received
                // --------------------------------------------

                if (received_percent < 0) {
                    received_percent = 0;
                }

                if (received_percent > 100) {
                    received_percent = 100;
                }


                percent =
                    received_percent;


                // --------------------------------------------
                // Show received value immediately
                // --------------------------------------------

                render_multizone_ui(
                    percent,
                    raw_value,
                    message
                );


                ESP_LOGI(
                    "BRIDGE",
                    "Received SET:%d:%s",
                    received_percent,
                    message
                );
            }
        }


        // ----------------------------------------------------
        // Loop delay
        // ----------------------------------------------------

        vTaskDelay(
            pdMS_TO_TICKS(50)
        );
    }
}