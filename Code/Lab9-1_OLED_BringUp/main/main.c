/*
 * ==========================================================================
 *  Lab 9.1 : SPI OLED Deconstructed Bring-up + Framebuffer Forensics
 *  Target  : ESP32 + OLED 0.96" SSD1306 (7-pin, 4-wire SPI)
 *  IDF     : v5.x / v6.x
 * --------------------------------------------------------------------------
 *  Wiring (ตามใบงาน 9.1)
 *    OLED D0  (SCK)  -> GPIO 18
 *    OLED D1  (MOSI) -> GPIO 23
 *    OLED RES        -> GPIO 4
 *    OLED DC         -> GPIO 2
 *    OLED CS         -> GPIO 5
 *    OLED VCC        -> 3.3V     (ห้ามต่อ 5V)
 *    OLED GND        -> GND
 * --------------------------------------------------------------------------
 *  หมายเหตุสำคัญ: ใบงาน 9.1 จงใจ "ไม่ส่ง" 0xA1 / 0xC8 เพื่อให้ภาพกลับหัว
 *  180 องศา แล้วนำไปชันสูตรต่อในใบงาน 9.4 (Case 3) โค้ดนี้จึงคงพฤติกรรมนั้นไว้
 * ==========================================================================
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"

#include "font5x7.h"

/* ----------------------------------------------------------------------- */
/* กิจกรรมที่ 1.1 : Low-Level SPI & DC Toggle                               */
/* ----------------------------------------------------------------------- */

/* 1. กำหนดขาเชื่อมต่อตามแผนภาพวงจรจริง */
#define OLED_PIN_SCK    (GPIO_NUM_18) /* D0 (SPI Clock)            */
#define OLED_PIN_MOSI   (GPIO_NUM_23) /* D1 (SPI MOSI Data)        */
#define OLED_PIN_RES    (GPIO_NUM_4)  /* RES (Hardware Reset)      */
#define OLED_PIN_DC     (GPIO_NUM_2)  /* DC (0 = Command, 1 = Data)*/
#define OLED_PIN_CS     (GPIO_NUM_5)  /* CS (Chip Select, Active LOW) */

#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_PAGES      (OLED_HEIGHT / 8)          /* 8 pages          */
#define OLED_BUF_SIZE   (OLED_WIDTH * OLED_PAGES)  /* 1,024 bytes = 1KB*/

static const char *TAG = "LAB9_1";

static spi_device_handle_t s_spi_handle = NULL;

/* Back Buffer 1KB ในแรมของ ESP32 (static = อยู่ใน internal RAM ใช้ DMA ได้) */
static uint8_t s_oled_buffer[OLED_BUF_SIZE];

/* มาโครช่วยพิมพ์เลขฐานสองสำหรับงาน Forensics */
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)        \
    ((byte) & 0x80 ? '1' : '0'),    \
    ((byte) & 0x40 ? '1' : '0'),    \
    ((byte) & 0x20 ? '1' : '0'),    \
    ((byte) & 0x10 ? '1' : '0'),    \
    ((byte) & 0x08 ? '1' : '0'),    \
    ((byte) & 0x04 ? '1' : '0'),    \
    ((byte) & 0x02 ? '1' : '0'),    \
    ((byte) & 0x01 ? '1' : '0')

/* 2. ฟังก์ชันกำหนดค่าเริ่มต้นพิน GPIO และบัสฮาร์ดแวร์ SPI2 */
esp_err_t oled_spi_init(void)
{
    /* กำหนดขา DC และ RES เป็น Output */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << OLED_PIN_DC) | (1ULL << OLED_PIN_RES),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    /* กำหนดค่าบัส SPI (Master Out Only - จอนี้ Write-Only ไม่มีขา MISO) */
    spi_bus_config_t buscfg = {
        .miso_io_num     = -1,
        .mosi_io_num     = OLED_PIN_MOSI,   /* GPIO 23 */
        .sclk_io_num     = OLED_PIN_SCK,    /* GPIO 18 */
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = OLED_BUF_SIZE + 16,
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ผูก Device เข้ากับ Bus (ความถี่ 10 MHz, SPI Mode 0) */
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000, /* 10 MHz              */
        .mode           = 0,                /* CPOL = 0, CPHA = 0  */
        .spics_io_num   = OLED_PIN_CS,      /* GPIO 5              */
        .queue_size     = 7,
    };

    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &s_spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

/* 3. ฟังก์ชันส่งคำสั่ง 1 ไบต์ (Command: DC = 0) */
void oled_send_cmd(uint8_t cmd)
{
    gpio_set_level(OLED_PIN_DC, 0);     /* LOW = Command */

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length    = 8;                    /* 8 บิต = 1 ไบต์ */
    t.tx_buffer = &cmd;
    ESP_ERROR_CHECK(spi_device_polling_transmit(s_spi_handle, &t));
}

/* 4. ฟังก์ชันส่งบล็อกข้อมูลพิกเซล (Data: DC = 1) */
void oled_send_data(const uint8_t *data, size_t len)
{
    if (len == 0) return;

    gpio_set_level(OLED_PIN_DC, 1);     /* HIGH = Pixel Data */

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length    = len * 8;              /* จำนวนบิตทั้งหมด */
    t.tx_buffer = data;
    ESP_ERROR_CHECK(spi_device_polling_transmit(s_spi_handle, &t));
}

/* ----------------------------------------------------------------------- */
/* กิจกรรมที่ 1.3 : Bitwise Canvas บน Framebuffer 1KB                       */
/* ----------------------------------------------------------------------- */

/* 1. ล้างหน้าจอในแรม (ดำสนิท) */
void oled_clear(void)
{
    memset(s_oled_buffer, 0x00, sizeof(s_oled_buffer));
}

/* 2. จุด/ดับพิกเซลด้วยคณิตศาสตร์ระดับบิต */
void oled_draw_pixel(int x, int y, bool color)
{
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;

    int byte_index = x + (y / 8) * OLED_WIDTH;  /* Index = x + floor(y/8)*128 */
    int bit_offset = y % 8;                     /* Bit   = y mod 8            */

    if (color) {
        s_oled_buffer[byte_index] |= (uint8_t)(1u << bit_offset);
    } else {
        s_oled_buffer[byte_index] &= (uint8_t)~(1u << bit_offset);
    }
}

/* 3. ส่งถ่าย 1,024 ไบต์จากแรมขึ้นจอจริง (Buffer Flush) */
void oled_flush(void)
{
    oled_send_cmd(0x21);            /* Set Column Address */
    oled_send_cmd(0x00);            /* Start Column 0     */
    oled_send_cmd(OLED_WIDTH - 1);  /* End Column 127     */

    oled_send_cmd(0x22);            /* Set Page Address   */
    oled_send_cmd(0x00);            /* Start Page 0       */
    oled_send_cmd(OLED_PAGES - 1);  /* End Page 7         */

    oled_send_data(s_oled_buffer, sizeof(s_oled_buffer));
}

/* ----------------------------------------------------------------------- */
/* กิจกรรมที่ 1.4 : Font Matrix 5x7                                         */
/* ----------------------------------------------------------------------- */

void oled_draw_char(int x, int y, char c, bool color)
{
    if (c < 32 || c > 126) c = '?';     /* นอกช่วง ASCII ที่พิมพ์ได้ */

    int font_idx = c - 32;

    for (int col = 0; col < 5; col++) {
        uint8_t line = font5x7[font_idx][col];
        for (int row = 0; row < 7; row++) {
            if (line & (1u << row)) {
                oled_draw_pixel(x + col, y + row, color);
            } else {
                oled_draw_pixel(x + col, y + row, !color);
            }
        }
    }
    /* ช่องไฟ (Kerning) 1 พิกเซล */
    for (int row = 0; row < 7; row++) {
        oled_draw_pixel(x + 5, y + row, !color);
    }
}

void oled_draw_string(int x, int y, const char *str, bool color)
{
    while (*str) {
        oled_draw_char(x, y, *str, color);
        x += 6;                      /* 5 พิกเซล + ช่องไฟ 1 พิกเซล */
        if (x + 6 > OLED_WIDTH) break;
        str++;
    }
}

/* ----------------------------------------------------------------------- */
/* กิจกรรมนิติวิทยาศาสตร์ 1.1 : Hex Dump Memory Inspection                  */
/* ----------------------------------------------------------------------- */

static void oled_forensic_dump(int start_index, int count)
{
    ESP_LOGI(TAG, "=== DUMPING FRAMEBUFFER PAGE 0 (%d Bytes from index %d) ===",
             count, start_index);
    for (int i = start_index; i < start_index + count && i < OLED_BUF_SIZE; i++) {
        printf("Byte[%4d] (Page %d, Col %3d): 0x%02X  [Binary: " BYTE_TO_BINARY_PATTERN "]\n",
               i, i / OLED_WIDTH, i % OLED_WIDTH,
               s_oled_buffer[i], BYTE_TO_BINARY(s_oled_buffer[i]));
    }
    ESP_LOGI(TAG, "=== END OF DUMP ===");
}

/* ----------------------------------------------------------------------- */
/* app_main : ครบทั้ง 6 ขั้นตอนตามใบงาน                                     */
/* ----------------------------------------------------------------------- */

void app_main(void)
{
    /* 1. เริ่มต้นระบบบัส SPI2 และตั้งค่าพิน DC/RES */
    ESP_ERROR_CHECK(oled_spi_init());
    ESP_LOGI(TAG, "SPI2 bus ready @ 10 MHz (Mode 0)");

    /* 2. ลำดับการ Hardware Reset (ขา RES, Active LOW) */
    gpio_set_level(OLED_PIN_RES, 0);
    vTaskDelay(pdMS_TO_TICKS(15));
    gpio_set_level(OLED_PIN_RES, 1);
    vTaskDelay(pdMS_TO_TICKS(15));

    /* 3. Magic Sequence (ฉบับย่อของใบงาน 9.1) */
    oled_send_cmd(0xAE);    /* step 1  : Display OFF                       */
    oled_send_cmd(0x8D);    /* step 6  : Charge Pump Setting               */
    oled_send_cmd(0x14);    /*           0x14 = Enable (0x10 = จอดับสนิท!) */
    oled_send_cmd(0x20);    /* step 7  : Memory Addressing Mode            */
    oled_send_cmd(0x00);    /*           0x00 = Horizontal Mode            */
    oled_send_cmd(0xAF);    /* step 16 : Display ON                        */
    /* ยังไม่ส่ง 0xA1 / 0xC8 -> ภาพจะกลับหัว (ตั้งใจไว้แก้ในใบงาน 9.4)     */

    /* 4. ทดสอบถมหน้าจอ (สว่างทั้งจอ 1,024 ไบต์) */
    uint8_t buffer[OLED_WIDTH];
    memset(buffer, 0xFF, sizeof(buffer));
    oled_send_cmd(0x21); oled_send_cmd(0x00); oled_send_cmd(0x7F);
    oled_send_cmd(0x22); oled_send_cmd(0x00); oled_send_cmd(0x07);
    for (int page = 0; page < OLED_PAGES; page++) {
        oled_send_data(buffer, sizeof(buffer));
    }
    ESP_LOGI(TAG, "Step 4: full-screen fill (proof of life)");
    vTaskDelay(pdMS_TO_TICKS(1500));

    /* 5. ทดสอบจุด 4 มุมจอ (Corner Pixels Test) */
    oled_clear();
    oled_draw_pixel(0,   0,  true);     /* บนซ้าย  */
    oled_draw_pixel(127, 0,  true);     /* บนขวา   */
    oled_draw_pixel(0,   63, true);     /* ล่างซ้าย */
    oled_draw_pixel(127, 63, true);     /* ล่างขวา  */
    oled_flush();
    ESP_LOGI(TAG, "Step 5: 4 corner pixels");
    vTaskDelay(pdMS_TO_TICKS(1500));

    /* 6. พิมพ์ข้อความ Hello World และรหัสนักศึกษา */
    oled_clear();
    oled_draw_string(30, 4,  "HELLO WORLD",  true);  /* โซนสีเหลือง */
    oled_draw_string(24, 32, "ID: 65012345", true);  /* โซนสีฟ้า    */
    oled_flush();
    ESP_LOGI(TAG, "Step 6: text rendered");

    /* 7. Framebuffer Forensics : ดัมพ์ไบต์ตัวอักษร 'H' ตัวแรก
     *    'H' เริ่มที่ x = 30, y = 4 -> อยู่ใน Page 0 คอลัมน์ 30..35        */
    vTaskDelay(pdMS_TO_TICKS(500));
    oled_forensic_dump(30, 16);
}
