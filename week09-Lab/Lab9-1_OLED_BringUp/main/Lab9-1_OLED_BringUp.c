#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "font5x7.h" // 

// ---------------------------------------------------------
// แมโครสำหรับกิจกรรมนิติวิทยาศาสตร์ (Forensics) แปลง Byte เป็น Binary String
// ---------------------------------------------------------
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


// =========================================================
// กิจกรรมที่ 1.1: การสร้างท่อส่งสัญญาณระดับล่าง (Low-Level SPI)
// =========================================================
#define OLED_PIN_SCK    (GPIO_NUM_18) 
#define OLED_PIN_MOSI   (GPIO_NUM_23) 
#define OLED_PIN_RES    (GPIO_NUM_4)  
#define OLED_PIN_DC     (GPIO_NUM_2)  
#define OLED_PIN_CS     (GPIO_NUM_5)  

static spi_device_handle_t s_spi_handle = NULL;

esp_err_t oled_spi_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << OLED_PIN_DC) | (1ULL << OLED_PIN_RES),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    spi_bus_config_t buscfg = {
        .miso_io_num = -1,               
        .mosi_io_num = OLED_PIN_MOSI,     
        .sclk_io_num = OLED_PIN_SCK,      
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 1024 + 16,
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) return ret;

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000, // 10 MHz
        .mode = 0,                          
        .spics_io_num = OLED_PIN_CS,        
        .queue_size = 7,
    };

    return spi_bus_add_device(SPI2_HOST, &devcfg, &s_spi_handle);
}

void oled_send_cmd(uint8_t cmd)
{
    gpio_set_level(OLED_PIN_DC, 0); 
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8; 
    t.tx_buffer = &cmd;
    spi_device_polling_transmit(s_spi_handle, &t);
}

void oled_send_data(const uint8_t *data, size_t len)
{
    if (len == 0) return;
    gpio_set_level(OLED_PIN_DC, 1); 
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = len * 8; 
    t.tx_buffer = data;
    spi_device_polling_transmit(s_spi_handle, &t);
}


// =========================================================
// กิจกรรมที่ 1.3: เอนจินพิกเซลบน 1KB Framebuffer (Bitwise Canvas)
// =========================================================
static uint8_t s_oled_buffer[1024]; 

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
    oled_send_cmd(0x21); 
    oled_send_cmd(0x00); 
    oled_send_cmd(0x7F); 

    oled_send_cmd(0x22); 
    oled_send_cmd(0x00); 
    oled_send_cmd(0x07); 

    oled_send_data(s_oled_buffer, sizeof(s_oled_buffer));
}


// =========================================================
// กิจกรรมที่ 1.4: สร้างตัวอักษรและพิมพ์ข้อความ
// =========================================================
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
    
    for (int row = 0; row < 7; row++) {
        oled_draw_pixel(x + 5, y + row, !color);
    }
}

void oled_draw_string(int x, int y, const char *str, bool color)
{
    while (*str) {
        oled_draw_char(x, y, *str, color);
        x += 6; 
        if (x + 6 > 128) break; 
        str++;
    }
}


// =========================================================
// ฟังก์ชันหลัก (Main Task) รวมการแสดงผลและการทำ Forensics
// =========================================================
void app_main(void)
{
    // 1. เริ่มต้นระบบบัส SPI2
    ESP_ERROR_CHECK(oled_spi_init());

    // 2. Hardware Reset
    gpio_set_level(OLED_PIN_RES, 0); 
    vTaskDelay(pdMS_TO_TICKS(15));
    gpio_set_level(OLED_PIN_RES, 1); 
    vTaskDelay(pdMS_TO_TICKS(15));

    // 3. กิจกรรมที่ 1.2: Magic Sequence ปลุกหน้าจอ
    oled_send_cmd(0xAE); // Display OFF
    oled_send_cmd(0x8D); // Charge Pump
    oled_send_cmd(0x14); // Enable Charge Pump
    oled_send_cmd(0x20); // Addressing Mode
    oled_send_cmd(0x00); // Horizontal Mode
    oled_send_cmd(0xAF); // Display ON

    // 4. ทดสอบถมหน้าจอสีขาว 1.5 วินาที
    uint8_t buffer[128];
    memset(buffer, 0xFF, sizeof(buffer));
    oled_send_cmd(0x21); oled_send_cmd(0x00); oled_send_cmd(0x7F);
    oled_send_cmd(0x22); oled_send_cmd(0x00); oled_send_cmd(0x07);
    for (int page = 0; page < 8; page++) {
        oled_send_data(buffer, sizeof(buffer));
    }
    vTaskDelay(pdMS_TO_TICKS(1500)); 

    // 5. กิจกรรมที่ 1.3: จุดพิกเซล 4 มุมจอ 1.5 วินาที
    oled_clear();
    oled_draw_pixel(0, 0, true);     
    oled_draw_pixel(127, 0, true);   
    oled_draw_pixel(0, 63, true);    
    oled_draw_pixel(127, 63, true);  
    oled_flush();
    vTaskDelay(pdMS_TO_TICKS(1500)); 

    // 6. กิจกรรมที่ 1.4: พิมพ์ Hello World และ รหัสนักศึกษา
    oled_clear();
    oled_draw_string(30, 4, "HELLO WORLD", true);   
    // อย่าลืมแก้ไขรหัสนักศึกษาตรงนี้
    oled_draw_string(24, 32, "ID: 67030195", true); 
    oled_flush();

    // 7. กิจกรรมนิติวิทยาศาสตร์ 1.1: Hex Dump
    // ดัชนีเริ่มต้นที่ 30 เพื่อให้ตรงกับตำแหน่งแกน x ของตัวอักษร 'H' พอดี
    ESP_LOGI("FORENSIC", "=== DUMPING FRAMEBUFFER (Text Area) ===");
    for (int i = 30; i < 30 + 16; i++) {
        printf("Byte[%2d] (Col %2d): 0x%02X  [Binary: " BYTE_TO_BINARY_PATTERN "]\n", 
               i, i, s_oled_buffer[i], BYTE_TO_BINARY(s_oled_buffer[i]));
    }
}