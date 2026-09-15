#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_err.h"

// นำเข้าไฟล์แบบอักษรขนาด 5x7 พิกเซล (ต้องวางไฟล์ font5x7.h ไว้ในโฟลเดอร์ main/)
#include "font5x7.h"

#define TAG "OLED_LAB9_1"

// -----------------------------------------------------------------------------
// 1. กำหนดพินเชื่อมต่อ (Pinout Mapping)
// -----------------------------------------------------------------------------
#define OLED_PIN_SCK    (GPIO_NUM_18) // D0 (SPI Clock)
#define OLED_PIN_MOSI   (GPIO_NUM_23) // D1 (SPI MOSI Data)
#define OLED_PIN_RES    (GPIO_NUM_4)  // RES (Hardware Reset)
#define OLED_PIN_DC     (GPIO_NUM_2)  // DC (0 = Command, 1 = Data)
#define OLED_PIN_CS     (GPIO_NUM_5)  // CS (Chip Select - Active LOW)

// แมโครสำหรับแปลงไบต์เป็นเลขฐานสองเพื่อใช้พิมพ์ในงาน Forensics
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')

// -----------------------------------------------------------------------------
// 2. Global Variables & Framebuffer
// -----------------------------------------------------------------------------
static spi_device_handle_t s_spi_handle = NULL;
static uint8_t s_oled_buffer[1024]; // 128 คอลัมน์ x 8 เพจ (64 บิตแนวตั้ง) = 1,024 ไบต์

// -----------------------------------------------------------------------------
// 3. Low-Level SPI & GPIO Driver
// -----------------------------------------------------------------------------
esp_err_t oled_spi_init(void)
{
    // กำหนดขา DC และ RES เป็น Output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << OLED_PIN_DC) | (1ULL << OLED_PIN_RES),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // กำหนดค่าบัส SPI2 (VSPI)
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,               // จอนี้ Write-Only ไม่ใช้งาน MISO
        .mosi_io_num = OLED_PIN_MOSI,     // GPIO 23
        .sclk_io_num = OLED_PIN_SCK,      // GPIO 18
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 1024 + 16,
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) return ret;

    // ผูก Device เข้ากับ Bus
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000, // 10 MHz
        .mode = 0,                          // Mode 0: CPOL=0, CPHA=0
        .spics_io_num = OLED_PIN_CS,        // GPIO 5
        .queue_size = 7,
    };

    return spi_bus_add_device(SPI2_HOST, &devcfg, &s_spi_handle);
}

void oled_send_cmd(uint8_t cmd)
{
    gpio_set_level(OLED_PIN_DC, 0); // DC = 0: ส่ง Command
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &cmd;
    spi_device_polling_transmit(s_spi_handle, &t);
}

void oled_send_data(const uint8_t *data, size_t len)
{
    if (len == 0) return;
    gpio_set_level(OLED_PIN_DC, 1); // DC = 1: ส่ง Pixel Data
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = len * 8;
    t.tx_buffer = data;
    spi_device_polling_transmit(s_spi_handle, &t);
}

// -----------------------------------------------------------------------------
// 4. Bitwise Framebuffer Engine
// -----------------------------------------------------------------------------
void oled_clear(void)
{
    memset(s_oled_buffer, 0x00, sizeof(s_oled_buffer));
}

void oled_draw_pixel(int x, int y, bool color)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;

    int byte_index = x + (y / 8) * 128;
    int bit_offset = y % 8;

    if (color) {
        s_oled_buffer[byte_index] |= (1 << bit_offset);
    } else {
        s_oled_buffer[byte_index] &= ~(1 << bit_offset);
    }
}

void oled_flush(void)
{
    // กำหนดขอบเขตคอลัมน์ 0 ถึง 127
    oled_send_cmd(0x21);
    oled_send_cmd(0x00);
    oled_send_cmd(0x7F);

    // กำหนดขอบเขตเพจ 0 ถึง 7
    oled_send_cmd(0x22);
    oled_send_cmd(0x00);
    oled_send_cmd(0x07);

    // ถ่ายโอนบัฟเฟอร์ขนาด 1KB ขึ้นสู่หน่วยความจำของจอ
    oled_send_data(s_oled_buffer, sizeof(s_oled_buffer));
}

// -----------------------------------------------------------------------------
// 5. Font Matrix 5x7 Engine
// -----------------------------------------------------------------------------
void oled_draw_char(int x, int y, char c, bool color)
{
    if (c < 32 || c > 126) c = '?';

    int font_idx = c - 32;

    for (int col = 0; col < 5; col++) {
        uint8_t line = font5x7[font_idx][col];
        for (int row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                oled_draw_pixel(x + col, y + row, color);
            } else {
                oled_draw_pixel(x + col, y + row, !color);
            }
        }
    }
    // ช่องไฟระหว่างตัวอักษร 1 พิกเซล
    for (int row = 0; row < 7; row++) {
        oled_draw_pixel(x + 5, y + row, !color);
    }
}

void oled_draw_string(int x, int y, const char *str, bool color)
{
    while (*str) {
        oled_draw_char(x, y, *str, color);
        x += 6; // กว้าง 5 + ช่องไฟ 1
        if (x + 6 > 128) break;
        str++;
    }
}

// -----------------------------------------------------------------------------
// 6. Main Application Workflow
// -----------------------------------------------------------------------------
void app_main(void)
{
    // ขั้นตอนที่ 1: เริ่มต้นระบบบัส SPI2 และตั้งค่าพิน
    ESP_LOGI(TAG, "Initializing SPI Bus and GPIO...");
    ESP_ERROR_CHECK(oled_spi_init());

    // ขั้นตอนที่ 2: ลำดับการ Hardware Reset
    ESP_LOGI(TAG, "Executing Hardware Reset...");
    gpio_set_level(OLED_PIN_RES, 0);
    vTaskDelay(pdMS_TO_TICKS(15));
    gpio_set_level(OLED_PIN_RES, 1);
    vTaskDelay(pdMS_TO_TICKS(15));

    // ขั้นตอนที่ 3: ส่งชุดคำสั่ง Magic Sequence เปิด Charge Pump และเปิดจอ
    ESP_LOGI(TAG, "Sending Initialization Magic Sequence...");
    oled_send_cmd(0xAE); // Display OFF
    oled_send_cmd(0x8D); // Set Charge Pump
    oled_send_cmd(0x14); // Enable Charge Pump (0x14)
    oled_send_cmd(0x20); // Set Memory Addressing Mode
    oled_send_cmd(0x00); // Horizontal Addressing Mode
    oled_send_cmd(0xAF); // Display ON

    // ขั้นตอนที่ 4: ทดสอบถมหน้าจอ (สว่างเต็มจอทั้ง 1,024 ไบต์)
    ESP_LOGI(TAG, "Step 4: All-pixels Fill Test...");
    uint8_t buffer[128];
    memset(buffer, 0xFF, sizeof(buffer));
    oled_send_cmd(0x21); oled_send_cmd(0x00); oled_send_cmd(0x7F);
    oled_send_cmd(0x22); oled_send_cmd(0x00); oled_send_cmd(0x07);
    for (int page = 0; page < 8; page++) {
        oled_send_data(buffer, sizeof(buffer));
    }
    vTaskDelay(pdMS_TO_TICKS(1500)); // ค้างไว้ 1.5 วินาที

    // ขั้นตอนที่ 5: ทดสอบจุด 4 มุมจอ (Corner Pixels Test)
    ESP_LOGI(TAG, "Step 5: Corner Pixels Test...");
    oled_clear();
    oled_draw_pixel(0, 0, true);     // มุมบนซ้าย (โซนเหลือง)
    oled_draw_pixel(127, 0, true);   // มุมบนขวา (โซนเหลือง)
    oled_draw_pixel(0, 63, true);    // มุมล่างซ้าย (โซนฟ้า)
    oled_draw_pixel(127, 63, true);  // มุมล่างขวา (โซนฟ้า)
    oled_flush();
    vTaskDelay(pdMS_TO_TICKS(1500)); // ค้างไว้ 1.5 วินาที

    // ขั้นตอนที่ 6: พิมพ์ข้อความ HELLO WORLD และ รหัสนักศึกษา
    ESP_LOGI(TAG, "Step 6: Text Rendering...");
    oled_clear();
    oled_draw_string(30, 4, "HELLO WORLD", true);  // โซนเหลือง
    oled_draw_string(24, 32, "ID: 65012345", true); // โซนฟ้า
    oled_flush();

    // -------------------------------------------------------------------------
    // กิจกรรมนิติวิทยาศาสตร์ (Framebuffer Forensics)
    // ตรวจสอบ Memory Dump ช่วงคอลัมน์ 30-45 (ตำแหน่งของตัว 'H' และ 'E')
    // -------------------------------------------------------------------------
    ESP_LOGI("FORENSIC", "=== DUMPING FRAMEBUFFER PAGE 0 (Columns 30 to 45) ===");
    for (int i = 30; i < 46; i++) {
        printf("Byte[%3d] (Col %3d): 0x%02X  [Binary: " BYTE_TO_BINARY_PATTERN "]\n", 
               i, i, s_oled_buffer[i], BYTE_TO_BINARY(s_oled_buffer[i]));
    }
}