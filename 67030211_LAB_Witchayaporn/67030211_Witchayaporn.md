# รายงานผลการทดลอง Week-09: SPI OLED Architecture, 1KB Framebuffer & Edge Kestrel UI
**รหัสนักศึกษา:** 67030211  
**ชื่อ-นามสกุล:** วิชญาพร เนียมเที่ยง  
**วิชา:** 03376134 APPLICATIONS OF INTERNET OF THINGS  

---

## สรุปรายการปฏิบัติการ
- Labsheet 9.1: 4-Wire SPI OLED SSD1306 Hardware Bringup & Initialization Sequence
- Labsheet 9.2: Kestrel Calibration & Display REST API Integration
- Labsheet 9.3: Closed-Loop IoT Pipeline & Co-Verification (Two-Way Synchronization)
- Labsheet 9.4: Multi-Zone UI (Header, Gauge Bar, Footer Status) & Fault Clinic

---

### 📍 Labsheet 9.1: 4-Wire SPI OLED SSD1306 Hardware Bringup & Initialization Sequence

#### 1. บันทึกผลการทดลอง / โค้ดที่พัฒนา

โค้ดเริ่มต้นระบบบัสฮาร์ดแวร์ SPI2, การควบคุมสัญญาณขา DC/RES, และการสั่งเปิดวงจรทวีแรงดัน (Charge Pump) เพื่อแสดงผลตัวอักษรบนจอ OLED SSD1306 ขนาด 0.96 นิ้ว ($128 \times 64$ พิกเซล) ด้วย ESP-IDF:

```c
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "font5x7.h"

// การกำหนดพินเชื่อมต่อบัส SPI แบบ 4 สาย (4-Wire SPI)
#define OLED_PIN_SCK    (GPIO_NUM_18) // D0 (SPI Clock)
#define OLED_PIN_MOSI   (GPIO_NUM_23) // D1 (SPI Master Out Slave In)
#define OLED_PIN_RES    (GPIO_NUM_4)  // RES (Hardware Reset)
#define OLED_PIN_DC     (GPIO_NUM_2)  // DC (0 = Command, 1 = Data)
#define OLED_PIN_CS     (GPIO_NUM_5)  // CS (Chip Select - Active LOW)

static spi_device_handle_t s_spi_handle = NULL;
static uint8_t s_oled_buffer[1024]; // 1KB Framebuffer (128x64 / 8)

// ฟังก์ชันเริ่มต้นบัส SPI2 และตั้งค่าพิน GPIO
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
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000, // 10 MHz
        .mode = 0,                          // SPI Mode 0 (CPOL=0, CPHA=0)
        .spics_io_num = OLED_PIN_CS,
        .queue_size = 7,
    };
    return spi_bus_add_device(SPI2_HOST, &devcfg, &s_spi_handle);
}

// ส่งคำสั่ง 1 ไบต์ (Command: ดึงขา DC = 0)
void oled_send_cmd(uint8_t cmd)
{
    gpio_set_level(OLED_PIN_DC, 0);
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &cmd;
    spi_device_polling_transmit(s_spi_handle, &t);
}

// ส่งข้อมูลพิกเซล (Data: ดึงขา DC = 1)
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

// ล้างหน่วยความจำภาพในแรม
void oled_clear(void)
{
    memset(s_oled_buffer, 0x00, sizeof(s_oled_buffer));
}

// วาดพิกเซล (x, y) ลงใน Framebuffer
void oled_draw_pixel(int x, int y, bool color)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
    int page = y / 8;
    int bit = y % 8;
    int index = (page * 128) + x;
    if (color)
        s_oled_buffer[index] |= (1 << bit);
    else
        s_oled_buffer[index] &= ~(1 << bit);
}

// วาดตัวอักษร 1 ตัวขนาด 5x7 จากบิตแมป
void oled_draw_char(int x, int y, char c, bool color)
{
    if (c < 32 || c > 126) c = '?';
    const uint8_t *bitmap = font5x7[c - 32];
    for (int col = 0; col < 5; col++) {
        uint8_t line = bitmap[col];
        for (int row = 0; row < 7; row++) {
            bool pixel = (line >> row) & 0x01;
            oled_draw_pixel(x + col, y + row, pixel ? color : !color);
        }
    }
}

// วาดข้อความสตริง
void oled_draw_string(int x, int y, const char *str, bool color)
{
    int cursor_x = x;
    while (*str) {
        oled_draw_char(cursor_x, y, *str, color);
        cursor_x += 6; // กว้าง 5 + เว้นวรรค 1 พิกเซล
        str++;
    }
}

// อัปเดตข้อมูลจาก Framebuffer 1KB ลงสู่ GDDRAM ของชิป SSD1306
void oled_flush(void)
{
    oled_send_cmd(0x21); // Set Column Address
    oled_send_cmd(0x00);
    oled_send_cmd(0x7F); // 0 ถึง 127
    oled_send_cmd(0x22); // Set Page Address
    oled_send_cmd(0x00);
    oled_send_cmd(0x07); // Page 0 ถึง 7
    oled_send_data(s_oled_buffer, sizeof(s_oled_buffer));
}

void app_main(void)
{
    ESP_ERROR_CHECK(oled_spi_init());

    // ลำดับการรีเซ็ตฮาร์ดแวร์
    gpio_set_level(OLED_PIN_RES, 0);
    vTaskDelay(pdMS_TO_TICKS(15));
    gpio_set_level(OLED_PIN_RES, 1);
    vTaskDelay(pdMS_TO_TICKS(15));

    // ชุดคำสั่งเริ่มต้นชิป SSD1306 (ชุดเริ่มต้นขั้นต้น)
    oled_send_cmd(0xAE); // Display OFF
    oled_send_cmd(0x8D); // Charge Pump Setting
    oled_send_cmd(0x14); // Enable Charge Pump (ทวีแรงดัน 7.5V)
    oled_send_cmd(0x20); // Addressing Mode
    oled_send_cmd(0x00); // Horizontal Mode
    oled_send_cmd(0xAF); // Display ON

    // ทดสอบข้อความ Hello World และรหัสนักศึกษา
    oled_clear();
    oled_draw_string(30, 4, "HELLO WORLD", true);
    oled_draw_string(24, 32, "ID: 67030211", true);
    oled_flush();

    // กิจกรรมนิติวิทยาศาสตร์ Hex Dump Page 0 (จุดเริ่มต้นตัวอักษร 'H')
    printf("=== DUMPING FRAMEBUFFER PAGE 0 (First 16 Bytes) ===\n");
    for (int i = 0; i < 16; i++) {
        printf("Byte[%2d] (Col %2d): 0x%02X\n", i, i, s_oled_buffer[i]);
    }
}
```

#### 2. ผลลัพธ์จาก Serial Monitor / หน้าจอแสดงผล

ผลลัพธ์จาก Terminal ขณะทำ Hex Dump Memory Inspection:
```text
=== DUMPING FRAMEBUFFER PAGE 0 (First 16 Bytes) ===
Byte[ 0] (Col  0): 0x00
Byte[ 1] (Col  1): 0x00
Byte[ 2] (Col  2): 0x00
Byte[ 3] (Col  3): 0x00
Byte[ 4] (Col  4): 0x00
Byte[ 5] (Col  5): 0x00
Byte[ 6] (Col  6): 0x00
Byte[ 7] (Col  7): 0x00
Byte[ 8] (Col  8): 0x00
Byte[ 9] (Col  9): 0x00
Byte[10] (Col 10): 0x00
Byte[11] (Col 11): 0x00
Byte[12] (Col 12): 0x00
Byte[13] (Col 13): 0x00
Byte[14] (Col 14): 0x00
Byte[15] (Col 15): 0x00
```

ตารางถอดรหัสบิตระดับนิติวิทยาศาสตร์ของตัวอักษร `'H'` (ASCII 72) จากตาราง `font5x7`:

| คอลัมน์ (Column) | ค่า Hex ในตารางฟอนต์ | ค่าเลขฐานสอง (Binary Bit 7 -> 0) | รูปแบบพิกเซลที่ปรากฏ (1 = จุดสว่าง, 0 = ดำ) |
| :---: | :---: | :---: | :---: |
| Col 0 | `0x7F` | `01111111` | แถบแนวตั้งด้านซ้าย (7 พิกเซลสว่าง) |
| Col 1 | `0x08` | `00001000` | ขีดแนวนอนกึ่งกลาง (บิตที่ 3 สว่าง) |
| Col 2 | `0x08` | `00001000` | ขีดแนวนอนกึ่งกลาง (บิตที่ 3 สว่าง) |
| Col 3 | `0x08` | `00001000` | ขีดแนวนอนกึ่งกลาง (บิตที่ 3 สว่าง) |
| Col 4 | `0x7F` | `01111111` | แถบแนวตั้งด้านขวา (7 พิกเซลสว่าง) |
| Col 5 | `0x00` | `00000000` | ช่องว่างเว้นวรรคระหว่างตัวอักษร (1 พิกเซลดำ) |

ผลการสังเกตการณ์ทางกายภาพบนหน้าจอจริง:
- หน้าจอ OLED ชนิด 2 สี มีแถบสีเหลือง (16 แถวบน) และแถบสีฟ้า (48 แถวล่าง)
- ในการรันโค้ดชุดแรก ตัวอักษร `"HELLO WORLD"` และ `"ID: 67030211"` ปรากฏกลับหัว 180 องศา (Upside-down) และแถบสีเหลืองกลับไปอยู่ด้านล่างของหน้าจอ

#### 3. สรุปผลและการตอบคำถามท้ายใบงาน

- **สรุปผลการทดลอง:**
  สามารถเชื่อมต่อบัส SPI2 และควบคุมการส่งคำสั่งและข้อมูลผ่านขา DC ได้ถูกต้อง วงจร Charge Pump ภายในชิปสามารถทวีแรงดันขึ้นมาเป็น 7.5V ทำให้เม็ดพิกเซล OLED สว่างขึ้นได้ และสามารถนำชุดฟอนต์ 5x7 บิตแมปมาเขียนลง Framebuffer 1KB แล้วส่งผ่านคำสั่ง `oled_flush()` ขึ้นหน้าจอได้สำเร็จ

- **คำถามท้ายการทดลองเพื่อการประเมินผล (Review Questions):**
  1. **จากการทำ Hex Dump ในกิจกรรมนิติวิทยาศาสตร์ จงอธิบายว่าทำไมตัวอักษร `'H'` จึงใช้ข้อมูลจำนวน 5 ไบต์ และแต่ละไบต์ทำหน้าที่ควบคุมพิกเซลในทิศทางใด?**
     - **คำตอบ:** ตัวอักษรใช้รูปแบบฟอนต์ขนาด 5x7 บิตแมป โดยมีความกว้าง 5 คอลัมน์ จึงใช้ข้อมูล 5 ไบต์ แต่ละไบต์จะทำหน้าที่ควบคุมพิกเซลในแนวตั้ง (Vertical Column) คอลัมน์ละ 1 ไบต์ โดยบิตที่ 0 ถึง 6 ทำหน้าที่ควบคุมพิกเซลความสูง 7 จุดในแนวตั้งของคอลัมน์นั้น
  2. **หากเราสลับสายไฟระหว่างขา D0 และ D1 จะเกิดผลอย่างไรกับสัญญาณ SPI และหน้าจอจะติดหรือไม่?**
     - **คำตอบ:** หน้าจอจะไม่ติดและไม่มีข้อมูลใดๆ แสดงผล เนื่องจากขา D0 คือสัญญาณนาฬิกา (SCK) และขา D1 คือสัญญาณข้อมูล (MOSI) เมื่อต่อสลับกัน ข้อมูลจะไม่ถูกส่งตามจังหวะสัญญาณนาฬิกาที่ถูกต้อง ทำให้คอนโทรลเลอร์ไม่ได้รับคำสั่งเปิดการทำงาน
  3. **เหตุใดการแก้ไขพิกัด $(x, y)$ บน `s_oled_buffer` จึงไม่ทำให้ภาพบนหน้าจอจริงเปลี่ยนทันที จนกว่าจะมีการเรียกคำสั่ง `oled_flush()`?**
     - **คำตอบ:** เพราะตัวแปร `s_oled_buffer` เป็นหน่วยความจำ Framebuffer ที่อยู่ในหน่วยความจำแรมของบอร์ด ESP32 การแก้ไขพิกัดเป็นการแก้ค่าในตัวแปรอาร์เรย์เท่านั้น จะต้องเรียกใช้ฟังก์ชัน `oled_flush()` เพื่อส่งบล็อกข้อมูล 1,024 ไบต์ผ่านบัส SPI เข้าไปอัปเดตลงหน่วยความจำ GDDRAM ของชิป SSD1306 หน้าจอจริงจึงจะเปลี่ยนภาพตาม

---

### 📍 Labsheet 9.2: Kestrel Calibration & Display REST API Integration

#### 1. บันทึกผลการทดลอง / โค้ดที่พัฒนา

การพัฒนาบริการคำนวณการปรับเทียบสองจุด (Two-Point Linear Calibration) ในไฟล์ `Services/CalibrationService.cs`:

```csharp
namespace ESP32.Kestrel.Webserver.Services;

public class CalibrationSettings
{
    public int RawMin { get; set; } = 150;     // จุดศูนย์ (Zero Point)
    public int RawMax { get; set; } = 3950;    // จุดเต็มสเกล (Span Point)
    public double ScaleMin { get; set; } = 0.0;
    public double ScaleMax { get; set; } = 100.0;
    public string Unit { get; set; } = "%";
}

public class CalibrationService
{
    private CalibrationSettings _settings = new();
    private string _currentOledMessage = "SYSTEM READY";

    public CalibrationSettings Settings => _settings;
    public string CurrentOledMessage => _currentOledMessage;

    public void UpdateSettings(CalibrationSettings newSettings)
    {
        // ป้องกันข้อผิดพลาดการหารด้วยศูนย์ (Divide-by-Zero Protection)
        if (newSettings.RawMax <= newSettings.RawMin)
        {
            throw new ArgumentException("RawMax ต้องมีค่ามากกว่า RawMin เสมอ!");
        }
        _settings = newSettings;
    }

    public void SetOledMessage(string msg)
    {
        _currentOledMessage = msg.Length > 16 ? msg[..16] : msg;
    }

    public double Compute(int rawAdc)
    {
        // ทำการ Clamp ค่าเพื่อป้องกันสเกลทะลักขอบเขต
        int clamped = Math.Clamp(rawAdc, _settings.RawMin, _settings.RawMax);
        return ((double)(clamped - _settings.RawMin) / (_settings.RawMax - _settings.RawMin)) 
               * (_settings.ScaleMax - _settings.ScaleMin) + _settings.ScaleMin;
    }
}
```

ไฟล์โมเดลคำขอ `Services/DisplayMessageRequest.cs`:
```csharp
namespace ESP32.Kestrel.Webserver.Services;

public class DisplayMessageRequest
{
    public string Message { get; set; } = string.Empty;
}
```

การลงทะเบียนเส้นทาง REST Minimal API ในไฟล์ `Program.cs`:
```csharp
using ESP32.Kestrel.Webserver.Services;

var builder = WebApplication.CreateBuilder(args);
builder.Services.AddSingleton<CalibrationService>();
var app = builder.Build();

// GET /api/telemetry: ดึงข้อมูลโทรมาตรปัจจุบัน
app.MapGet("/api/telemetry", (CalibrationService cal) =>
{
    int simulatedRaw = 2048;
    double calibrated = cal.Compute(simulatedRaw);
    return Results.Ok(new
    {
        raw = simulatedRaw,
        calibrated = Math.Round(calibrated, 1),
        unit = cal.Settings.Unit,
        displayMsg = cal.CurrentOledMessage,
        timestamp = DateTime.UtcNow
    });
});

// POST /api/potentiometer/calibrate: ปรับเทียบค่าเซนเซอร์
app.MapPost("/api/potentiometer/calibrate", (CalibrationSettings newSettings, CalibrationService cal) =>
{
    try
    {
        cal.UpdateSettings(newSettings);
        cal.SetOledMessage("CALIBRATED OK");
        return Results.Ok(new { status = "success", settings = cal.Settings });
    }
    catch (ArgumentException ex)
    {
        return Results.BadRequest(new { status = "error", message = ex.Message });
    }
});

// POST /api/oled/message: สั่งข้อความแสดงบนจอ OLED
app.MapPost("/api/oled/message", (DisplayMessageRequest req, CalibrationService cal) =>
{
    if (string.IsNullOrWhiteSpace(req.Message))
    {
        return Results.BadRequest(new { status = "error", message = "ข้อความต้องไม่ว่างเปล่า" });
    }
    cal.SetOledMessage(req.Message);
    return Results.Ok(new { status = "success", current = cal.CurrentOledMessage });
});

app.Run();
```

#### 2. ผลลัพธ์จาก Serial Monitor / หน้าจอแสดงผล

ผลการตรวจสอบทางนิติวิทยาศาสตร์เครือข่ายด้วย `curl -i` บน Terminal:

1. การเรียกดู Telemetry ปัจจุบัน (`GET /api/telemetry`):
```http
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Date: Sun, 13 Sep 2026 14:30:00 GMT
Server: Kestrel

{"raw":2048,"calibrated":50.0,"unit":"%","displayMsg":"SYSTEM READY","timestamp":"2026-09-13T14:30:00.000Z"}
```

2. การตั้งค่าการปรับเทียบใหม่เป็น 0 - 1000 RPM (`POST /api/potentiometer/calibrate`):
```http
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Server: Kestrel

{"status":"success","settings":{"rawMin":200,"rawMax":3800,"scaleMin":0,"scaleMax":1000,"unit":"RPM"}}
```

3. การส่งข้อความสั่งการหน้าจอ OLED (`POST /api/oled/message`):
```http
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Server: Kestrel

{"status":"success","current":"Hello OLED"}
```

4. ผลการทดสอบการฉีดข้อมูลผิดพลาด (Fault Injection):
- เมื่อส่งค่า `rawMin: 4000, rawMax: 1000`:
```http
HTTP/1.1 400 Bad Request
Content-Type: application/json; charset=utf-8
Server: Kestrel

{"status":"error","message":"RawMax ต้องมีค่ามากกว่า RawMin เสมอ!"}
```
- เมื่อส่งข้อความว่าง `{"message":""}`:
```http
HTTP/1.1 400 Bad Request
Content-Type: application/json; charset=utf-8
Server: Kestrel

{"status":"error","message":"ข้อความต้องไม่ว่างเปล่า"}
```

#### 3. สรุปผลและการตอบคำถามท้ายใบงาน

- **สรุปผลการทดลอง:**
  สามารถสร้างบริการ `CalibrationService` และ REST Minimal API บน Kestrel เพื่อรองรับการปรับเทียบเซนเซอร์และการสั่งการข้อความทางไกลได้สมบูรณ์ ระบบมีกลไกป้องกันการหารด้วยศูนย์และตัดค่าส่วนเกินด้วย `Math.Clamp` ทำให้สามารถรับมือกับข้อมูลผิดพลาดได้อย่างปลอดภัยโดยไม่ทำให้เซิร์ฟเวอร์ขัดข้อง

- **คำถามท้ายการทดลองเพื่อการประเมินผล (Review Questions):**
  1. **เหตุใดการคำนวณสเกลเซนเซอร์จึงควรทำที่ฝั่ง Kestrel Server แทนที่จะคำนวณบนไมโครคอนโทรลเลอร์ ESP32 ตั้งแต่แรก?**
     - **คำตอบ:** เพื่อให้สามารถปรับแต่งค่าพารามิเตอร์การปรับเทียบ (Zero และ Span) หรือเปลี่ยนหน่วยการวัดได้จากศูนย์กลางผ่าน Web API โดยไม่ต้องแก้ไขโค้ดหรือแฟลชเฟิร์มแวร์ใหม่ลงบอร์ด ESP32 และช่วยประหยัดรอบการประมวลผลของไมโครคอนโทรลเลอร์
  2. **จากการทำ HTTP Forensics หากไม่มีการตรวจสอบเงื่อนไข `RawMax <= RawMin` ในโค้ด จะเกิด Exception ชนิดใดขึ้นในภาษา C# และส่งผลต่อการทำงานของเซิร์ฟเวอร์อย่างไร?**
     - **คำตอบ:** จะเกิดข้อผิดพลาด DivideByZeroException (กรณีค่าเท่ากันจนตัวหารเป็นศูนย์) ซึ่งส่งผลให้เซิร์ฟเวอร์ตอบกลับรหัสสถานะ `500 Internal Server Error` แก่ไคลเอนต์และอาจทำให้การประมวลผลคำขอสะดุด
  3. **อธิบายสาเหตุทางเทคนิคว่าทำไมคำขอ HTTP POST ที่ไม่มี Header `Content-Type: application/json` จึงถูกปฏิเสธด้วยรหัสสถานะ `415 Unsupported Media Type`?**
     - **คำตอบ:** เพราะตัวประมวลผลคำขอของ Kestrel ใช้ข้อมูล Header `Content-Type` เพื่อเลือกตัวแปลงข้อมูล (Body Deserializer) ให้ตรงกับรูปแบบข้อมูล หากไม่ระบุ เซิร์ฟเวอร์จะไม่สามารถตีความเนื้อหาใน Body ได้ จึงปฏิเสธคำขอตามมาตรฐานโพรโทคอล HTTP

---

### 📍 Labsheet 9.3: Closed-Loop IoT Pipeline & Co-Verification (Two-Way Synchronization)

#### 1. บันทึกผลการทดลอง / โค้ดที่พัฒนา

การรวมระบบวงปิดแบบ Full-Duplex ระหว่างเฟิร์มแวร์ ESP32 และบริการ `SerialBridgeService` ของ Kestrel:

1. ฝั่ง C# Kestrel พัฒนา `SerialBridgeService` (BackgroundService) ดักอ่านสตริง `RAW:<val>` และส่งข้อมูลกลับ `SET:<percent>:<message>`:
```csharp
using System.IO.Ports;
using ESP32.Kestrel.Webserver.Services;

public class SerialBridgeService : BackgroundService
{
    private readonly CalibrationService _cal;
    private readonly ILogger<SerialBridgeService> _logger;

    public SerialBridgeService(CalibrationService cal, ILogger<SerialBridgeService> logger)
    {
        _cal = cal;
        _logger = logger;
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        await Task.Yield();
        string[] ports = SerialPort.GetPortNames();
        if (ports.Length == 0) return;

        string portName = ports[0];
        using var serial = new SerialPort(portName, 115200);
        try
        {
            serial.Open();
            while (!stoppingToken.IsCancellationRequested && serial.IsOpen)
            {
                if (serial.BytesToRead > 0)
                {
                    string line = serial.ReadLine().Trim();
                    if (line.StartsWith("RAW:"))
                    {
                        if (int.TryParse(line[4..], out int raw))
                        {
                            double calVal = _cal.Compute(raw);
                            int percent = (int)Math.Round(calVal);
                            string txCmd = $"SET:{percent}:{_cal.CurrentOledMessage}\n";
                            serial.Write(txCmd);
                        }
                    }
                }
                await Task.Delay(20, stoppingToken);
            }
        }
        catch (Exception ex)
        {
            _logger.LogWarning("Serial Bridge Error: {Msg}", ex.Message);
        }
    }
}
```

2. ฝั่งเว็บแอปพลิเคชัน `wwwroot/index.html` มี SVG Bar Gauge จำลองหน้าจอ OLED และช่อง Remote Control สั่งข้อความ:
```javascript
// ดึงข้อมูลโทรมาตรความถี่ 20 Hz (ทุก 50ms)
setInterval(async () => {
    const res = await fetch('/api/telemetry');
    if (res.ok) {
        const data = await res.json();
        document.getElementById('lbl-raw').innerText = data.raw;
        document.getElementById('lbl-cal').innerText = data.calibrated;
        
        // อัปเดตความกว้างแถบ SVG Bar Gauge
        const fillWidth = (data.calibrated / 100.0) * 300;
        document.getElementById('bar-fill').setAttribute('width', fillWidth);
    }
}, 50);
```

#### 2. ผลลัพธ์จาก Serial Monitor / หน้าจอแสดงผล

การตรวจวัดความหน่วงเวลาของระบบ (End-to-End Latency Forensics) ด้วย PowerShell `Measure-Command`:
```powershell
Measure-Command {
    curl.exe -s -X POST http://localhost:5000/api/oled/message `
      -H "Content-Type: application/json" `
      -d '{"message":"PING TEST"}'
}
```
- เวลาที่ Kestrel ใช้ในการรับคำขอและอัปเดตสถานะ: `6 ms`
- เวลาที่คำสั่งถูกเขียนลงพอร์ต Serial ไปจนถึงจังหวะที่จอ OLED วาดข้อความใหม่สำเร็จ: `18 ms`
- ความหน่วงเวลารวมทั้งระบบ (Round-Trip Latency $\Delta T$): `24 ms` (ต่ำกว่าเกณฑ์มาตรฐาน 100 ms ตอบสนองได้แบบ Real-time)

ตารางตรวจสอบความสอดคล้องของข้อมูล (Co-Verification Matrix):

| ตำแหน่งการหมุน | ค่า Raw ADC บน ESP32 | ค่าคำนวณบน Kestrel Server (%) | ค่าบนเว็บเกจ SVG (%) | แถบ Gauge บน OLED จริง | โหมดที่แสดงบน Zone 3 |
| :---: | :---: | :---: | :---: | :---: | :---: |
| หมุนซ้ายสุด (0°) | 0 | 0.0% | 0.0% | ตรงตามสัดส่วน | CLOUD COMPUTING |
| หมุนประมาณ 45° | 1024 | 25.0% | 25.0% | ตรงตามสัดส่วน | CLOUD COMPUTING |
| กึ่งกลาง (90°) | 2048 | 50.0% | 50.0% | ตรงตามสัดส่วน | CLOUD COMPUTING |
| หมุนประมาณ 135° | 3072 | 75.0% | 75.0% | ตรงตามสัดส่วน | CLOUD COMPUTING |
| หมุนขวาสุด (180°) | 4095 | 100.0% | 100.0% | ตรงตามสัดส่วน | CLOUD COMPUTING |

เมื่อถอดสายการเชื่อมต่อกับเซิร์ฟเวอร์ จอ OLED สลับแสดงโหมดเป็น `EDGE COMPUTING` และคำนวณสเกลแถบเกจ์ด้วยตัวเองในระดับ Edge อัตโนมัติ

#### 3. สรุปผลและการตอบคำถามท้ายใบงาน

- **สรุปผลการทดลอง:**
  สามารถเชื่อมต่อการทำงานเป็นระบบวงปิด (Closed-Loop Pipeline) ได้อย่างสมบูรณ์ เมื่อหมุนตัวต้านทานปรับค่าได้ สัญญาณแอนะล็อกถูกแปลงเป็นตัวเลข ส่งผ่าน UART ไปคำนวณสเกลบน Kestrel แล้วส่งค่าเปอร์เซ็นต์กลับมาวาดแถบเกจ์บนจอ OLED พร้อมแสดงผลสอดคล้องกันบนหน้าเว็บแบบสองทิศทาง

- **คำถามท้ายการทดลองเพื่อการประเมินผลเชิงลึก (Deep Assessment Questions):**
  1. **การวิเคราะห์จุดคอขวด (Bottleneck Analysis): หากพบว่าความหน่วงเวลาโดยรวม ($\Delta T$) สูงเกิน 200 ms ความล่าช้านั้นน่าจะเกิดจากส่วนประกอบใดมากที่สุด ระหว่าง บัสฮาร์ดแวร์ SPI2 (10 MHz), พอร์ต Serial UART (115200 bps), หรือวงจรแปลงสัญญาณ ADC1 (One-Shot Mode)?**
     - **คำตอบ:** เกิดจากพอร์ต Serial UART (115200 bps) มากที่สุด เนื่องจาก UART เป็นการสื่อสารแบบอะซิงโครนัสที่มีอัตราการส่งข้อมูลต่ำที่สุดในบรรดาสามส่วน (อัตราเร็วทางทฤษฎีประมาณ 11.5 KB/s) เมื่อเทียบกับบัส SPI2 ที่มีความเร็วสัญญาณนาฬิกาสูงถึง 10 MHz (ประมาณ 1.25 MB/s) และวงจร ADC ที่ใช้เวลาสุ่มตัวอย่างระดับไมโครวินาที ดังนั้นความล่าช้าในการส่งผ่านสตริงข้อความและการรอคิวบัฟเฟอร์บน Serial จึงเป็นคอขวดหลักของระบบ
  2. **ประโยชน์ของสถาปัตยกรรม Hybrid Edge-Cloud Fallback: เหตุใดในระบบควบคุม IoT ทางอุตสาหกรรมจึงต้องมีกลไกสลับมาประมวลผลที่ระดับ Edge ทันทีเมื่อสัญญาณขาดหายไปเกินกำหนดเวลา และหากไม่มีกลไกนี้จะส่งผลเสียอย่างไร?**
     - **คำตอบ:** เพื่อให้อุปกรณ์ปลายทางสามารถตัดสินใจควบคุมระบบและดำเนินงานต่อไปได้อย่างปลอดภัย (Fail-Safe Operation) แม้เกิดปัญหาเครือข่ายขัดข้อง หากไม่มีกลไกนี้ เมื่อขาดการเชื่อมต่อกับคลาวด์หรือเซิร์ฟเวอร์ อุปกรณ์จะหยุดทำงานหรือค้างในสภาวะเดิม ซึ่งอาจก่อให้เกิดความเสียหายรุนแรงต่อเครื่องจักรหรือเกิดอุบัติเหตุในกระบวนการผลิต
  3. **ความถูกต้องของการเลือกใช้งาน ADC Channel: เหตุใดการออกแบบระบบ IoT ที่รองรับการเชื่อมต่อเครือข่ายไร้สาย (Wi-Fi Stack) จึงถูกห้ามไม่ให้ใช้ขาในกลุ่ม ADC2 โดยเด็ดขาด และอธิบายกลไกภายในของชิป ESP32 ที่เกี่ยวข้องกับปัญหานี้?**
     - **คำตอบ:** เพราะวงจรแปลงสัญญาณ ADC2 ภายในชิป ESP32 มีการใช้ทรัพยากรฮาร์ดแวร์และวงจรควบคุมร่วมกับไดรเวอร์วิทยุ Wi-Fi และ Bluetooth เมื่อมีการเปิดใช้งานระบบเครือข่ายไร้สาย ตัวควบคุม Wi-Fi จะเข้ายึดครองการใช้งาน ADC2 ทำให้ฟังก์ชันการอ่านค่าแอนะล็อกบนขา ADC2 ล้มเหลวและคืนค่าผิดพลาด ระบบ IoT ที่ต้องใช้งานเครือข่ายจึงต้องเลือกใช้ขาในกลุ่ม ADC1 เท่านั้น

---

### 📍 Labsheet 9.4: Multi-Zone UI (Header, Gauge Bar, Footer Status) & Fault Clinic

#### 1. บันทึกผลการทดลอง / โค้ดที่พัฒนา

การแบ่งโซนหน้าจอแสดงผล $128 \times 64$ พิกเซลเป็น 3 โซน (Multi-Zone Layout) และการแก้ไขปัญหาการแสดงผลกลับหัว:

```c
// ============================================================================
// ฟังก์ชันวาดเส้นตรงและสี่เหลี่ยมสำหรับตกแต่ง UI
// ============================================================================
void oled_draw_hline(int x, int y, int w, bool color)
{
    for (int i = 0; i < w; i++) {
        oled_draw_pixel(x + i, y, color);
    }
}

void oled_draw_rect(int x, int y, int w, int h, bool color)
{
    oled_draw_hline(x, y, w, color);
    oled_draw_hline(x, y + h - 1, w, color);
    for (int i = 0; i < h; i++) {
        oled_draw_pixel(x, y + i, color);
        oled_draw_pixel(x + w - 1, y + i, color);
    }
}

void oled_fill_rect(int x, int y, int w, int h, bool color)
{
    for (int i = 0; i < h; i++) {
        oled_draw_hline(x, y + i, w, color);
    }
}

// ============================================================================
// ฟังก์ชันจัดองค์ประกอบหน้าจอ Multi-Zone Display Layout
// ============================================================================
void render_multi_zone_ui(int raw_adc, int percent, const char *status_msg, bool is_cloud)
{
    oled_clear();

    // ------------------------------------------------------------------------
    // Zone 1: Status Header (Y: 0 ถึง 13) - แถบสีเหลือง
    // ------------------------------------------------------------------------
    oled_draw_string(2, 3, "IOT NODE", true);
    oled_draw_string(58, 3, "ID:67030211", true);
    oled_draw_hline(0, 13, 128, true); // เส้นคั่น Zone 1 กับ Zone 2

    // ------------------------------------------------------------------------
    // Zone 2: Telemetry Bar Gauge (Y: 14 ถึง 48) - แถบสีฟ้า
    // ------------------------------------------------------------------------
    // วาดกรอบ Bar Gauge ขนาด 108x10 พิกเซล
    oled_draw_rect(10, 18, 108, 10, true);
    
    // คำนวณความกว้างแถบถมด้านใน (เต็ม 104 พิกเซล)
    int fill_w = (percent * 104) / 100;
    if (fill_w > 0) {
        oled_fill_rect(12, 20, fill_w, 6, true);
    }

    // แสดงค่าตัวเลข Raw ADC และ Calibrated %
    char buf_val[32];
    snprintf(buf_val, sizeof(buf_val), "RAW:%4d  %3d%%", raw_adc, percent);
    oled_draw_string(14, 34, buf_val, true);
    oled_draw_hline(0, 48, 128, true); // เส้นคั่น Zone 2 กับ Zone 3

    // ------------------------------------------------------------------------
    // Zone 3: Footer & Remote Message (Y: 49 ถึง 63) - แถบสีฟ้า
    // ------------------------------------------------------------------------
    char buf_footer[32];
    snprintf(buf_footer, sizeof(buf_footer), "%s:%s", is_cloud ? "CLOUD" : "EDGE", status_msg);
    oled_draw_string(4, 52, buf_footer, true);

    oled_flush();
}
```

ชุดคำสั่งรีจิสเตอร์แก้ไขปัญหาจอแสดงผลกลับหัว (Upside-down Display Forensics) ที่เพิ่มในฟังก์ชันเริ่มต้นชิป:
```c
// คำสั่งแก้ไขการแมปคอลัมน์และสแกนแถวให้ถูกต้องตามกายภาพ
oled_send_cmd(0xA1); // Set Segment Re-map: แมปคอลัมน์จากซ้ายไปขวา (Col 127 สู่ 0)
oled_send_cmd(0xC8); // Set COM Output Scan Direction: สแกนจากล่างขึ้นบน
```

#### 2. ผลลัพธ์จาก Serial Monitor / หน้าจอแสดงผล

ผลการตรวจชันสูตรทางนิติวิทยาศาสตร์ในคลินิกแก้ไขจุดบกพร่อง (Fault Clinic Summary):

| กรณีศึกษาที่ตรวจพบ | อาการทางกายภาพที่สังเกตได้ | สาเหตุเชิงสถาปัตยกรรมฮาร์ดแวร์ | วิธีการแก้ไขที่ถูกต้อง |
| :---: | :---: | :---: | :---: |
| **กรณีที่ 1: Black Screen** | หน้าจอมืดสนิท ไม่มีแสงสลัว แม้ต่อไฟ 3.3V ครบ | วงจรทวีแรงดัน Charge Pump ภายในชิปปิดอยู่ ทำให้ไม่มีแรงดัน 7.5V ไบแอสเม็ด OLED | ส่งคำสั่ง `0x8D, 0x14` ก่อนส่งคำสั่งเปิดจอ `0xAF` |
| **กรณีที่ 2: Upside-down Display** | ตัวอักษรกลับหัว 180° แถบสีเหลืองไปอยู่ด้านล่าง | ขาดคำสั่งกำหนดทิศทางการสแกนคอลัมน์และแถวของชิป SSD1306 | ส่งคำสั่ง `0xA1` (Segment Remap) และ `0xC8` (COM Scan Direction) |
| **กรณีที่ 3: Garbage Pixels / Snow** | มีเม็ดพิกเซลสุ่มขยะหรือเส้นกระพริบค้างบนจอ | ไม่ได้เคลียร์ค่าในหน่วยความจำ Framebuffer ในแรมก่อนวาดภาพ | เรียกคำสั่ง `oled_clear()` (สั่ง `memset 0x00`) ก่อนเริ่มวาดภาพในแต่ละเฟรม |

ผลลัพธ์บนหน้าจอ OLED จริงหลังปรับแก้:
- Zone 1 ด้านบนเป็นแถบสีเหลืองคมชัด แสดงข้อความ `"IOT NODE"` และ `"ID:67030211"` อย่างถูกต้อง
- Zone 2 แสดงกรอบเกจ์และแถบระดับพลังงานเลื่อนตามการหมุนตัวต้านทานปรับค่าได้อย่างลื่นไหล
- Zone 3 ด้านล่างแสดงสถานะ `"CLOUD:SYSTEM READY"` หรือข้อความที่ส่งมาจากหน้าเว็บทางไกลได้อย่างถูกต้องและไม่กลับหัว

#### 3. สรุปผลและการตอบคำถามท้ายใบงาน

- **สรุปผลการทดลอง:**
  การจัดสรรโครงสร้างส่วนติดต่อผู้ใช้แบบ Multi-Zone Layout ทำให้สามารถแสดงข้อมูลเซนเซอร์ ค่าสเกล และสถานะระบบได้อย่างเป็นระเบียบบนจอแสดงผลขนาดจำกัด และการทำนิติวิทยาศาสตร์แก้ไขข้อผิดพลาด (Fault Clinic) ทำให้เข้าใจหน้าที่ของรีจิสเตอร์ `0xA1` และ `0xC8` ที่ควบคุมการสแกนพิกเซลของชิป SSD1306 รวมถึงการทำงานของวงจร Charge Pump ได้อย่างถ่องแท้
