# Code — Week 09: SPI OLED & Kestrel UI

โค้ดที่ส่งสำหรับใบงาน 9.1, 9.2 และ 9.3 · **รหัสนักศึกษา 67030098** · branch `67030098-HW`

---

## 1. [`Lab9-1_OLED_BringUp/`](Lab9-1_OLED_BringUp/) — ใบงาน 9.1

เฟิร์มแวร์ ESP-IDF ขับจอ SSD1306 ผ่าน SPI แบบเขียนไดรเวอร์เองทั้งหมด ไม่ใช้ไลบรารีสำเร็จรูป

```
Lab9-1_OLED_BringUp/
├── CMakeLists.txt
└── main/
    ├── CMakeLists.txt
    ├── font5x7.h        ← ตารางฟอนต์ ASCII 5x7 (คัดลอกจาก Assests/)
    └── main.c           ← ไดรเวอร์ + app_main ครบ 6 ขั้นตอน + Forensic Dump
```

**สิ่งที่โค้ดทำ**

| ฟังก์ชัน | หน้าที่ |
| :--- | :--- |
| `oled_spi_init()` | ตั้งค่า GPIO (DC, RES) และบัส SPI2 ที่ 10 MHz Mode 0 |
| `oled_send_cmd()` / `oled_send_data()` | ส่งคำสั่ง (DC = 0) และข้อมูลพิกเซล (DC = 1) |
| `oled_clear()` / `oled_draw_pixel()` / `oled_flush()` | จัดการ Framebuffer 1,024 ไบต์ |
| `oled_draw_char()` / `oled_draw_string()` | เรนเดอร์ตัวอักษรจาก Font Matrix 5x7 |
| `oled_forensic_dump()` | ดัมพ์ไบต์และบิตในแรมออก Serial Monitor |

### วิธีคอมไพล์และแฟลช

```powershell
cd Lab9-1_OLED_BringUp
idf.py set-target esp32
idf.py build
idf.py -p COM24 flash monitor
```

ผ่าน Docker (ไม่ต้องติดตั้ง ESP-IDF)

```powershell
docker run --rm -w /workspace/ --mount "type=bind,source=$((Get-Location).Path),target=/workspace" espressif/idf:release-v6.1 idf.py build
```

**ลำดับภาพที่ต้องเห็น:** จอขาวทั้งแผ่น 1.5 วิ → จุด 4 มุมจอ 1.5 วิ → ข้อความ `HELLO WORLD` + `ID: 67030098`

> ⚠️ ภาพจะแสดง **กลับหัว 180 องศา** ซึ่งเป็นพฤติกรรมที่ถูกต้องตามที่ใบงาน 9.1 กำหนดไว้
> (จงใจละคำสั่ง `0xA1` และ `0xC8` เพื่อนำไปวิเคราะห์ต่อ — ดูคำอธิบายใน [Answer 9.1](../Answer/answer-lab-9-1.md))

---

## 2. [`Lab9-2_Kestrel_Webserver/`](Lab9-2_Kestrel_Webserver/) — ใบงาน 9.2

Kestrel Web Server (.NET Minimal API) ทำหน้าที่ Calibration Engine และ Display Control API

```
Lab9-2_Kestrel_Webserver/
└── ESP32.Kestrel.Webserver/
    ├── Program.cs                        ← ผูก Route ทั้ง 3 เส้นทาง
    ├── Services/
    │   ├── CalibrationService.cs         ← Two-Point Linear Calibration
    │   └── DisplayMessageRequest.cs      ← DTO ของ POST /api/oled/message
    └── Properties/launchSettings.json    ← ตั้งพอร์ตไว้ที่ 5117
```

### API ที่ให้บริการ

| Method | Path | หน้าที่ |
| :--- | :--- | :--- |
| `GET` | `/api/telemetry` | คืน Raw ADC, ค่าที่ปรับเทียบแล้ว, หน่วย และข้อความปัจจุบัน |
| `POST` | `/api/potentiometer/calibrate` | ตั้งค่า Zero & Span (`rawMin`, `rawMax`, `scaleMin`, `scaleMax`, `unit`) |
| `POST` | `/api/oled/message` | ส่งข้อความ Broadcast ไปยังจอ OLED |

### วิธีรัน

```bash
cd Lab9-2_Kestrel_Webserver/ESP32.Kestrel.Webserver
dotnet run
```

เปิดเบราว์เซอร์ที่ `http://localhost:5117/` จะเห็น `Hello World!`

### ทดสอบเร็ว

```powershell
curl.exe -i http://localhost:5117/api/telemetry
```

ชุดคำสั่งทดสอบครบทุกกรณี (รวม Fault Injection 4 แบบ) อยู่ในหัวข้อ 5 ของ [Answer 9.2](../Answer/answer-lab-9-2.md)

---

## 3. [`Lab9-3_ClosedLoop/`](Lab9-3_ClosedLoop/) — ใบงาน 9.3

ระบบ IoT วงปิดแบบสมบูรณ์: Potentiometer → ESP32 ADC1 → Kestrel (Full-Duplex Serial Bridge) → จอ OLED + Web Dashboard พร้อมกลไก **Hybrid Edge-Cloud Fallback**

```
Lab9-3_ClosedLoop/
├── firmware/Lab9-3-ESP32-ClosedLoop/     ← เฟิร์มแวร์ ESP-IDF
│   ├── CMakeLists.txt
│   └── main/
│       ├── CMakeLists.txt                ← แก้บั๊ก SRCS จากใบงานให้ตรงชื่อไฟล์จริง
│       ├── font5x7.h
│       └── main.c
└── server/ESP32.Kestrel.ClosedLoop/      ← ต่อยอด Kestrel จาก Lab 9.2
    ├── Program.cs
    ├── Services/
    │   ├── CalibrationService.cs         ← เพิ่ม lock กัน Race Condition
    │   ├── DisplayMessageRequest.cs
    │   └── SerialBridgeService.cs        ← Two-Way Serial Bridge (BackgroundService)
    ├── appsettings.json                  ← ตั้งพอร์ต COM ที่ SerialPort:PortName
    └── wwwroot/index.html                ← Dashboard SVG + badge สถานะการเชื่อมต่อ
```

**โปรโตคอล Serial (115200 8N1):**

| ทิศทาง | รูปแบบ | ความถี่ |
| :--- | :--- | :---: |
| ESP32 → Kestrel | `ADC:<raw>,<uptime_ms>\n` | 20 Hz |
| Kestrel → ESP32 | `SET:<percent>:<message>\n` | ตามรอบที่รับข้อมูลเข้า |

**Hybrid Edge-Cloud Fallback:** ถ้า ESP32 ไม่ได้รับ `SET:` ภายใน 1,500 ms จะสลับไปคำนวณเปอร์เซ็นต์เองแบบ Local Edge Scaling ทันที และแสดง `EDGE: LOCAL EDGE` แทน `CLOUD: ...`

### วิธีรันเซิร์ฟเวอร์ (ทดสอบได้ทันที ไม่ต้องมีบอร์ด)

```bash
cd Lab9-3_ClosedLoop/server/ESP32.Kestrel.ClosedLoop
dotnet run
```

เปิดเบราว์เซอร์ที่ `http://localhost:5127/`

### วิธี build เฟิร์มแวร์ (ต้องมี ESP-IDF v6.x หรือ Docker)

```powershell
cd Lab9-3_ClosedLoop/firmware/Lab9-3-ESP32-ClosedLoop
idf.py set-target esp32
idf.py build
idf.py -p COM3 flash monitor
```

รายละเอียดบั๊กที่พบในโค้ดต้นฉบับของใบงาน (และวิธีแก้), ผลทดสอบเต็มรูปแบบ, Checklist Co-Verification และเฉลยคำถามท้ายบท อยู่ใน [Answer 9.3](../Answer/answer-lab-9-3.md)

---

## สภาพแวดล้อมที่ใช้พัฒนาและทดสอบ

| เครื่องมือ | เวอร์ชัน | สถานะการทดสอบ |
| :--- | :--- | :--- |
| .NET SDK | 10.0.400 | ✅ Build ผ่าน + รันทดสอบ API จริงครบทุก Endpoint (Lab 9.2 และ 9.3) |
| ESP-IDF | v6.x (ตามใบงาน) | เขียนตามสถาปัตยกรรม ESP-IDF v5/v6 (ยังไม่ได้คอมไพล์บนเครื่องนี้ เพราะไม่ได้ติดตั้ง toolchain — ทั้ง Lab 9.1 และ 9.3) |
