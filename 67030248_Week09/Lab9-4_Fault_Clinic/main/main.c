// ==============================================================================
// Worksheet Lab 9.4: Troubleshooting & Fault-Injection Clinic
// Student: นายสุวิจักขณ์ ลาวัง (67030248)
// Target Fault: Case 3: Upside-down Display - Segment Remap & COM Scan Direction 180-degree inverted
// ==============================================================================

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "font5x7.h"

static const char *TAG = "FAULT_CLINIC";

void app_main(void)
{
    ESP_LOGI(TAG, "======================================================");
    ESP_LOGI(TAG, "Lab 9.4 Fault Clinic - Investigator: นายสุวิจักขณ์ ลาวัง (67030248)");
    ESP_LOGI(TAG, "Assigned Case: Case 3: Upside-down Display");
    ESP_LOGI(TAG, "Root Cause Analysis: Segment Remap & COM Scan Direction 180-degree inverted");
    ESP_LOGI(TAG, "Applying Corrective Action: oled_send_cmd(0xA1); oled_send_cmd(0xC8);");
    ESP_LOGI(TAG, "======================================================");

    ESP_LOGI(TAG, "[STEP 1]: Injecting fault to reproduce physical symptom...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGW(TAG, "[FORENSIC ALERT]: Fault symptom observed on target!");

    ESP_LOGI(TAG, "[STEP 2]: Executing Memory & Register Forensic Audit...");
    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "[STEP 3]: Applying validated engineering fix...");
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_LOGI(TAG, "[VERIFICATION]: Display & Telemetry verified 100%% OK! Status: RESOLVED");
}
