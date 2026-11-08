# Week 09: ผลที่คาดว่าจะได้ (Lab 9.1 - 9.3)

> **สถานะ:** ส่วนที่เขียนว่า "ทดสอบบนคอมพิวเตอร์แล้ว" รันจริงโดยไม่มี ESP32 ส่วนอื่นเป็นผลที่คาดว่าจะได้ ยังไม่ได้ทดสอบกับบอร์ดและจอ OLED จริง

## ก่อนรัน

- ต่อวงจรตามใบงาน: OLED D0 = GPIO 18, D1 = GPIO 23, RES = GPIO 4, DC = GPIO 2, CS = GPIO 5, Potentiometer = GPIO 34 และจ่ายไฟ OLED 3.3V เท่านั้น
- Kestrel ใช้ `COM6` เป็นค่าเริ่มต้น ซึ่งในเครื่องนี้เป็น Bluetooth ให้ระบุพอร์ตจริงตอนรัน:

```powershell
dotnet run --SerialPort:PortName=COMx
```

## Lab 9.1: SPI OLED Bring-up (คาดการณ์)

ใช้โปรเจกต์ `Example_codes/Lab9-1_OLED_BringUp`

1. จอขาวเต็มจอประมาณ 1.5 วินาที
2. จุดทดสอบ 4 มุมประมาณ 1.5 วินาที
3. จอแสดง `HELLO WORLD` และ `ID: 67030116`

Hex Dump ตัวอักษร `H` ใช้ 5 ไบต์ตามฟอนต์ 5x7 ควรตรงกับค่าของ `H` ใน `font5x7.h`

ภาพบนจอเปลี่ยนหลังเรียก `oled_flush()` เท่านั้น เพราะ `oled_draw_*` แก้เฉพาะ Framebuffer ใน RAM

## Lab 9.2: Calibration & Display API (ทดสอบบนคอมพิวเตอร์แล้ว)

ผลจากการรันจริงโดยไม่ต่อบอร์ด (ค่า Telemetry จึงเป็น 0 และ `isConnected` เป็น `false`):

| การทดสอบ | HTTP | Response |
| :--- | :---: | :--- |
| `GET /api/telemetry` | 200 | `{"raw":0,"calibrated":0,"unit":"%","displayMsg":"SYSTEM READY","isConnected":false,...}` |
| `POST /api/potentiometer/calibrate` (ค่าถูกต้อง) | 200 | `{"status":"success","settings":{"rawMin":100,"rawMax":3900,...}}` |
| `RawMax` เท่ากับ `RawMin` | 400 | `RawMax ต้องมีค่ามากกว่า RawMin เสมอ!` |
| `Content-Type: text/plain` | 415 | ไม่มี Body |
| `POST /api/oled/message` | 200 | `{"status":"success","current":"HELLO 116"}` |
| ข้อความว่าง | 400 | `ข้อความต้องไม่ว่างเปล่า` |

## Lab 9.3: End-to-End Closed Loop (คาดการณ์)

**ขั้นที่ 1, ESP32 อย่างเดียว:**

- Serial แสดงรูปแบบ `ADC:<raw>,<uptime_ms>`
- OLED Header แสดง `ESP32 | 67030116`
- ค่า `RAW` และ Bar Gauge ตามการหมุน
- Footer แสดง `EDGE: LOCAL EDGE`

**ขั้นที่ 2, ต่อ Kestrel:**

- `isConnected` เป็น `true`, ค่า `raw` และ `calibrated` ตามการหมุน
- ส่งข้อความผ่าน `/api/oled/message` แล้วจอ OLED เปลี่ยนตาม
- ถอดการเชื่อมต่อ Kestrel แล้ว OLED ควรกลับไปโหมด Edge

**ตาราง Co-Verification** (ใช้ค่าเริ่มต้น RawMin 150, RawMax 3950 และตำแหน่งเป็นค่าประมาณ):

| ตำแหน่ง | Raw ADC โดยประมาณ | Kestrel (%) | Web | OLED | ผล |
| :---: | :---: | :---: | :---: | :---: | :---: |
| 0° | 0 - 150 | 0 | เท่ากับ Kestrel | เท่ากับ Kestrel | ตรง |
| 45° | ราว 1000 | ราว 22 | เท่ากับ Kestrel | เท่ากับ Kestrel | ตรง |
| 90° | ราว 2050 | ราว 50 | เท่ากับ Kestrel | เท่ากับ Kestrel | ตรง |
| 135° | ราว 3050 | ราว 76 | เท่ากับ Kestrel | เท่ากับ Kestrel | ตรง |
| 180° | 3950 - 4095 | 100 | เท่ากับ Kestrel | เท่ากับ Kestrel | ตรง |

ค่าอาจต่างกัน ±1 เพราะข้อมูลอัปเดตคนละจังหวะ และ `kestrelLatencyMs` ควรต่ำมาก เนื่องจากวัดเฉพาะการประมวลผลใน Kestrel ค่า Latency ของทั้งระบบต้องวัดตามวิธีในใบงาน

## โปรเจกต์ที่มีให้และสิ่งที่แก้

- `Lab9-1_OLED_BringUp`: ใส่รหัสนักศึกษาแทน `65012345`
- `Lab9-1-SPI-OLED-Bringup`, `Lab9-SPI-OLED-MultiZone`: โค้ดตัวอย่างของอาจารย์ ไม่ได้แก้
- `Lab9-3-ESP32-ClosedLoop`: ใส่รหัสนักศึกษาที่ Header
- `ESP32.Kestrel.Webserver`: ใช้สำหรับ Lab 9.2 และ 9.3 ไม่ได้แก้
