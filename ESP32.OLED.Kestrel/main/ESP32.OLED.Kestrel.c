#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#define TAG "OLED_KestREL"

// Potentiometer
#define POT_ADC_CHANNEL ADC_CHANNEL_6   // GPIO34

// OLED SPI2
#define OLED_MOSI 23
#define OLED_CLK  18
#define OLED_CS   5
#define OLED_DC   16
#define OLED_RST  17

static adc_oneshot_unit_handle_t adc1_handle;

static int current_percent = 0;
static char current_message[32] = "SYSTEM READY";

static void init_potentiometer(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1
    };

    ESP_ERROR_CHECK(
        adc_oneshot_new_unit(&init_config, &adc1_handle)
    );

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12
    };

    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(
            adc1_handle,
            POT_ADC_CHANNEL,
            &config
        )
    );
}

static int adc_to_percent(int raw)
{
    if (raw < 0)
        raw = 0;

    if (raw > 4095)
        raw = 4095;

    return (raw * 100) / 4095;
}

static void oled_spi_init(void)
{
    ESP_LOGI(TAG, "OLED SPI2 initialized");
}

static void oled_init_display(void)
{
    ESP_LOGI(TAG, "OLED display initialized");
}

static void render_multizone_ui(
    int percent,
    int raw,
    const char *message
)
{
    current_percent = percent;

    strncpy(
        current_message,
        message,
        sizeof(current_message) - 1
    );

    current_message[sizeof(current_message) - 1] = '\0';

    printf(
        "OLED | STATUS: ESP32 OK | RAW:%d | VALUE:%d%% | MSG:%s\n",
        raw,
        percent,
        current_message
    );
}

static int read_serial_line(
    char *buffer,
    size_t size
)
{
    if (fgets(buffer, size, stdin) == NULL)
        return 0;

    size_t len = strlen(buffer);

    if (len > 0 && buffer[len - 1] == '\n')
        buffer[len - 1] = '\0';

    return 1;
}

void app_main(void)
{
    oled_spi_init();
    oled_init_display();
    init_potentiometer();

    int raw_val = 0;

    char rx_buffer[64];

    while (1)
    {
        // อ่าน ADC
        ESP_ERROR_CHECK(
            adc_oneshot_read(
                adc1_handle,
                POT_ADC_CHANNEL,
                &raw_val
            )
        );

        // คำนวณเปอร์เซ็นต์
        int percent = adc_to_percent(raw_val);

        // ส่ง Raw ADC ไปยัง Kestrel
        printf("ADC:%d\n", raw_val);

        // ตรวจสอบข้อมูลจาก Kestrel
        if (read_serial_line(
                rx_buffer,
                sizeof(rx_buffer)))
        {
            int received_percent = 0;
            char msg[32] = {0};

            if (sscanf(
                    rx_buffer,
                    "%d,%31s",
                    &received_percent,
                    msg) == 2)
            {
                render_multizone_ui(
                    received_percent,
                    raw_val,
                    msg
                );
            }
        }

        vTaskDelay(
            pdMS_TO_TICKS(50)
        );
    }
}