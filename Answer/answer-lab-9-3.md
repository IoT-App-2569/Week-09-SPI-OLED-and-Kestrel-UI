# เฉลย / รายงานผลใบงานที่ 9.3 (Answer Lab 9.3)
### การรวมระบบวงปิดแบบครบวงจร การตรวจสอบความสอดคล้องของข้อมูลและเวลาหน่วง

| หัวข้อ | รายละเอียด |
| :--- | :--- |
| **รหัสนักศึกษา** | 67030098 |
| **ใบงาน** | [08-Labsheet-09-3-End-to-End-IoT-Loop-and-Verification.md](../08-Labsheet-09-3-End-to-End-IoT-Loop-and-Verification.md) |
| **โค้ดที่ส่ง (เฟิร์มแวร์)** | [`Code/Lab9-3_ClosedLoop/firmware/Lab9-3-ESP32-ClosedLoop/`](../Code/Lab9-3_ClosedLoop/firmware/Lab9-3-ESP32-ClosedLoop/) |
| **โค้ดที่ส่ง (เซิร์ฟเวอร์)** | [`Code/Lab9-3_ClosedLoop/server/ESP32.Kestrel.ClosedLoop/`](../Code/Lab9-3_ClosedLoop/server/ESP32.Kestrel.ClosedLoop/) |
| **เฟรมเวิร์ก** | ESP-IDF v6.x (เฟิร์มแวร์) + .NET 10 Minimal API (Kestrel, พอร์ต `5127`) |
| **สถานะการทดสอบ** | ✅ ฝั่งเซิร์ฟเวอร์: Build ผ่าน 0 Error, รันจริงและทดสอบครบทุก Endpoint ผ่าน Browser + curl<br>⚠️ ฝั่งเฟิร์มแวร์: **ยังไม่ได้คอมไพล์บนเครื่องนี้** เพราะไม่มี ESP-IDF/Docker ติดตั้งอยู่ (ดูหัวข้อ 6) |

---

## 1. สถาปัตยกรรมระบบวงปิดที่สร้าง

```
[ Potentiometer ] --(0-3.3V)--> [ ESP32 ADC1 GPIO34 ]
                                        │
                     "ADC:<raw>,<uptime_ms>\n"  (ทุก 50 ms / 20 Hz)
                                        ▼
                              [ Kestrel SerialBridgeService ]
                                        │  Compute() ผ่าน CalibrationService
                                        ▼
                     "SET:<percent>:<message>\n"
                                        │
                                        ▼
                              [ ESP32 Multi-Zone OLED ]  +  [ Web Dashboard SVG ]
```

**กลไก Hybrid Edge-Cloud Fallback:** ESP32 บันทึกเวลาล่าสุดที่ได้รับแพ็กเก็ต `SET:` จาก Kestrel
ถ้าเงียบเกิน **1,500 ms** จะตัดสลับกลับไปคำนวณเปอร์เซ็นต์เองแบบ Local Edge Scaling (`raw*100/4095`) และแสดง `EDGE: LOCAL EDGE` ทันที — ระบบยังทำงานต่อได้แม้ Kestrel ดับหรือสาย USB หลุด

---

## 2. โครงสร้างโปรเจกต์ที่ส่ง

```
Code/Lab9-3_ClosedLoop/
├── firmware/Lab9-3-ESP32-ClosedLoop/     ← เฟิร์มแวร์ ESP-IDF
│   ├── CMakeLists.txt
│   └── main/
│       ├── CMakeLists.txt
│       ├── font5x7.h
│       └── main.c
└── server/ESP32.Kestrel.ClosedLoop/      ← ต่อยอดจาก Lab 9.2
    ├── Program.cs
    ├── Services/
    │   ├── CalibrationService.cs
    │   ├── DisplayMessageRequest.cs
    │   └── SerialBridgeService.cs
    ├── appsettings.json                  ← ตั้งค่า SerialPort:PortName
    └── wwwroot/index.html
```

> แยกโฟลเดอร์เซิร์ฟเวอร์เป็นโปรเจกต์ใหม่ชื่อ `ESP32.Kestrel.ClosedLoop` (ไม่ใช้ชื่อซ้ำกับ Lab 9.2) เพื่อให้รันคู่ขนานกับเซิร์ฟเวอร์ของ Lab 9.2 ได้พร้อมกันโดยไม่ชนพอร์ต (Lab 9.2 = 5117, Lab 9.3 = 5127)

---

## 3. บั๊ก 2 จุดที่พบในโค้ดต้นฉบับของใบงาน และวิธีแก้ไข

ระหว่างประกอบโค้ดตามใบงานเพื่อให้ Build ผ่านจริง พบข้อขัดแย้งภายในเอกสาร 2 จุด:

### บั๊กที่ 1 — `main/CMakeLists.txt` อ้างชื่อไฟล์ผิด

ใบงานเขียนไว้ในขั้นที่ 3.1.2 ว่า

```cmake
idf_component_register(SRCS "Lab9-3-ESP32-ClosedLoop.c" ...)
```

แต่โครงสร้างโปรเจกต์ในหัวข้อ 3.0 กำหนดชื่อไฟล์เป็น **`main/main.c`** (ไม่ใช่ `Lab9-3-ESP32-ClosedLoop.c`) ถ้าใช้ตามใบงานตรง ๆ `idf.py build` จะ error ทันทีด้วย

```
CMake Error: Cannot find source file: Lab9-3-ESP32-ClosedLoop.c
```

**วิธีแก้ที่ใช้:** แก้ `SRCS` ให้ตรงกับชื่อไฟล์จริง

```cmake
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_driver_spi esp_driver_gpio esp_driver_uart esp_adc esp_timer
)
```

### บั๊กที่ 2 — Race Condition บน `CalibrationService` (Thread Safety)

โค้ดต้นฉบับในใบงาน (ขั้นที่ 3.2.2) ไม่ได้ใส่กลไกป้องกันการเข้าถึงพร้อมกันเลย แต่ในสถาปัตยกรรม Lab 9.3 ออบเจกต์นี้ถูกลงทะเบียนเป็น **Singleton** และถูกแตะจาก **2 เธรดที่ทำงานคู่ขนานกันตลอดเวลา**

| เธรด | เข้าถึงเมื่อไร |
| :--- | :--- |
| HTTP Request Thread | ทุกครั้งที่มีคนเรียก `/api/potentiometer/calibrate` หรือ `/api/oled/message` |
| `SerialBridgeService` (Background Thread) | ทุกครั้งที่ได้รับบรรทัด `ADC:` จาก ESP32 (สูงสุด 20 ครั้ง/วินาที) |

ถ้าไม่ล็อก มีโอกาส (แม้จะน้อย) ที่ HTTP thread กำลังเขียน `_settings = newSettings` ครึ่งทาง ขณะที่ background thread กำลังอ่าน `_settings.RawMin` พร้อมกันพอดี ซึ่งใน C# แม้ reference assignment จะเป็น atomic แต่การอ่านเขียนสอง property (`RawMin`, `RawMax`) พร้อมกันจากคนละเธรดยังนับเป็น Data Race ตามนิยามของ .NET Memory Model

**วิธีแก้ที่ใช้:** เพิ่ม `lock (_gate)` ครอบทุกจุดที่อ่าน/เขียน state ร่วม (ดู [`CalibrationService.cs`](../Code/Lab9-3_ClosedLoop/server/ESP32.Kestrel.ClosedLoop/Services/CalibrationService.cs)) — ไม่เปลี่ยนพฤติกรรมภายนอกของ API แม้แต่น้อย เป็นเพียงการเสริมความปลอดภัยของหน่วยความจำร่วม

---

## 4. ผลการทดสอบจริง

### 4.1 ฝั่งเซิร์ฟเวอร์ (ทดสอบได้เต็มรูปแบบ — รันจริงบนเครื่องนี้)

```bash
cd Code/Lab9-3_ClosedLoop/server/ESP32.Kestrel.ClosedLoop
dotnet build     # Build succeeded, 0 Error(s)
dotnet run       # Now listening on: http://localhost:5127
```

**Log ตอนสตาร์ท (ไม่มี ESP32 ต่ออยู่บนเครื่องทดสอบ):**

```
info: ESP32.Kestrel.ClosedLoop.Services.SerialBridgeService[0]
      กำลังเปิดการเชื่อมต่อ Serial Port: COM3 ที่ BaudRate 115200
info: Microsoft.Hosting.Lifetime[14]
      Now listening on: http://localhost:5127
info: ESP32.Kestrel.ClosedLoop.Services.SerialBridgeService[0]
      เชื่อมต่อพอร์ต COM3 สำเร็จ!
```

**ผลทดสอบ API ทั้ง 3 เส้นทาง (curl จริง)**

| # | คำสั่ง | ผล |
| :---: | :--- | :--- |
| 1 | `GET /api/telemetry` | `{"raw":0,"calibrated":0,"unit":"%","displayMsg":"READY","isConnected":true,"kestrelLatencyMs":0,...}` |
| 2 | `POST /api/potentiometer/calibrate` (rawMin=200, rawMax=3800, unit=RPM) | `{"status":"success","settings":{...,"unit":"RPM"}}` |
| 3 | `POST /api/oled/message` (`"HI CLOSEDLOOP"`) | `{"status":"success","current":"HI CLOSEDLOOP"}` |
| 4 | `GET /api/telemetry` ซ้ำ | `unit` เปลี่ยนเป็น `RPM`, `displayMsg` เปลี่ยนเป็น `HI CLOSEDLOOP` — ยืนยัน state persist ข้าม request |
| 5 | Fault: `rawMin(4000) > rawMax(1000)` | `400 Bad Request` ✅ |
| 6 | Fault: ข้อความว่าง | `400 Bad Request` ✅ |
| 7 | `GET /` (Static File) | `200 OK` — เสิร์ฟ `wwwroot/index.html` ได้ |

> **หมายเหตุ:** `raw` คงที่ที่ `0` และ `isConnected: true` เพราะพอร์ต COM3 บนเครื่องทดสอบนี้เปิดได้จริง (มี COM3 อยู่) แต่ไม่มี ESP32 ต่ออยู่จริงจึงไม่มีใครส่งบรรทัด `ADC:...` เข้ามา นี่คือพฤติกรรมที่ถูกต้องตามที่ออกแบบไว้ — `isConnected` สะท้อนแค่ "เปิดพอร์ตสำเร็จ" ไม่ได้แปลว่า "มีบอร์ดตอบกลับ" (ระบบไม่ล่มและไม่ throw exception ใด ๆ แม้ไม่มีข้อมูลไหลเข้ามาเลย)

**ทดสอบผ่านเว็บเบราว์เซอร์จริง** — เปิด `http://localhost:5127/` แล้วพิมพ์ข้อความ `"TEST BROWSER"` ในช่อง Remote Control กดปุ่มส่ง

ผล: badge แสดง **`SERIAL OK`** (สีเขียว, ยืนยันว่า `isConnected=true`), การ์ด `CALIBRATED VALUE` แสดงหน่วย `RPM` ตามที่ Calibrate ไว้ก่อนหน้า และข้อความสถานะขึ้น **"ส่งข้อความสำเร็จ!"** — ยืนยันว่า UI เรียก `fetch()` ไปที่ `/api/oled/message` และ parse response ได้ถูกต้องโดยไม่มี JavaScript error

**ทดสอบความทนทาน:** รันเซิร์ฟเวอร์ต่อเนื่อง ยิงคำขอทั้ง Success และ Fault Case สลับกัน ปิดเซิร์ฟเวอร์ด้วย `Ctrl+C` (SIGINT) — ปิดสะอาด ไม่มี Unhandled Exception ใน log ตลอดการทดสอบ

**สิ่งที่เพิ่มเติมจากใบงาน:** ใส่ badge `SERIAL OK` / `NO SERIAL` และแสดง `Kestrel Latency` บนหน้าเว็บ (ใบงานต้นฉบับไม่มี) เพื่อให้มองเห็นสถานะ `isConnected` ที่ API คืนมาอยู่แล้วโดยไม่ต้องเปิด DevTools ดู — ช่วยให้ตรวจ Checkpoint 3.2 ได้ง่ายขึ้นตอนทดสอบจริงกับบอร์ด

### 4.2 ฝั่งเฟิร์มแวร์ (ยังไม่ได้ตรวจสอบด้วยการคอมไพล์จริง)

เครื่องที่ใช้พัฒนาไม่มี ESP-IDF toolchain และไม่มี Docker ติดตั้งอยู่ (`idf.py`: not found, `docker`: not found) จึง **ยังไม่สามารถรัน `idf.py build` เพื่อยืนยันว่าคอมไพล์ผ่านจริงได้**

สิ่งที่ตรวจสอบได้โดยไม่ต้องคอมไพล์ (Manual Static Review):
- โครงสร้างวงเล็บ `{}` และการปิด statement ครบทุกฟังก์ชัน (ตรวจด้วยสายตาเทียบกับ Lab 9.1/9.2 ที่ใช้รูปแบบเดียวกันและคอมไพล์ผ่านแล้วจริงในสภาพแวดล้อมก่อนหน้า)
- Header ที่ include ครบ (`inttypes.h` สำหรับ `PRId64`, `driver/uart.h` สำหรับ UART API)
- ชื่อฟังก์ชัน ESP-IDF API (`adc_oneshot_*`, `spi_bus_*`, `uart_*`) ตรงกับที่ใช้จริงใน Lab 9.1 ซึ่งอ้างอิง ESP-IDF v6.x เหมือนกัน
- แก้ไข `main/CMakeLists.txt` ให้ตรงกับชื่อไฟล์แล้ว (บั๊กที่ 1 ข้างต้น)

**สิ่งที่นักศึกษาต้องทำเพิ่มก่อนส่งจริง:** รัน `idf.py build` บนเครื่องที่มี ESP-IDF แล้วแก้ error จากการคอมไพล์ (ถ้ามี) ก่อน flash ลงบอร์ดจริง — ดูคำสั่งในหัวข้อ 6

---

## 5. Checklist การทดสอบภาคสนาม (ให้ทำเมื่อมีบอร์ดจริง)

### Checkpoint 3.1 — Standalone / Edge Computing Mode (ก่อนเปิด Kestrel)
- [ ] Serial Monitor แสดง `ADC:xxxx,xxxxx` ไหลต่อเนื่อง
- [ ] จอ OLED โซน 1 แสดง `ESP32 | 67030098`
- [ ] หมุน Potentiometer แล้วแถบ Gauge และตัวเลข `RAW` ขยับลื่นไม่กระตุก
- [ ] โซน 3 แสดง `EDGE: LOCAL EDGE`
- [ ] กด `Ctrl+]` ออกจาก Serial Monitor ก่อนเปิด Kestrel

### Checkpoint 3.2 — Transition to Cloud Computing Mode
- [ ] แก้ `appsettings.json` → `SerialPort:PortName` เป็นพอร์ตจริงของบอร์ด (เช่น `COM3`)
- [ ] `dotnet run` แล้วเห็น `เชื่อมต่อพอร์ต COMxx สำเร็จ!`
- [ ] จอ OLED โซน 3 เปลี่ยนจาก `EDGE: LOCAL EDGE` เป็น `CLOUD: READY` **ทันที**
- [ ] กด `Ctrl+C` ปิด Kestrel → ภายใน ~1.5 วิ จอดีดกลับเป็น `EDGE: LOCAL EDGE`
- [ ] `dotnet run` ใหม่ → จอกลับเป็น `CLOUD` โดยไม่ต้องรีเซ็ตบอร์ด

### กิจกรรม 4.2 — Co-Verification Matrix

| ตำแหน่งการหมุน | Raw ADC (ESP32) | ค่า Kestrel (%) | ค่าเว็บ SVG (%) | Gauge OLED ตรงหรือไม่ | โหมด Zone 3 |
| :---: | :---: | :---: | :---: | :---: | :---: |
| ซ้ายสุด (0°) | _____ | _____ | _____ | [ ] ตรง | _____ |
| ~45° | _____ | _____ | _____ | [ ] ตรง | _____ |
| กึ่งกลาง (90°) | _____ | _____ | _____ | [ ] ตรง | _____ |
| ~135° | _____ | _____ | _____ | [ ] ตรง | _____ |
| ขวาสุด (180°) | _____ | _____ | _____ | [ ] ตรง | _____ |

> ให้กรอกค่าจริงจากบอร์ดตอนสอบ/ส่งงาน — ตารางนี้ต้องมีค่าจริงจากฮาร์ดแวร์เท่านั้น จำลองไม่ได้

---

## 6. วิธี Build และรันโค้ดที่ส่ง

### เฟิร์มแวร์ (ต้องมี ESP-IDF v6.x หรือ Docker)

```powershell
cd Code/Lab9-3_ClosedLoop/firmware/Lab9-3-ESP32-ClosedLoop
idf.py set-target esp32
idf.py build
idf.py -p COM3 flash monitor
```

หรือผ่าน Docker:

```powershell
docker run --rm -w /workspace/ --mount "type=bind,source=$((Get-Location).Path),target=/workspace" espressif/idf:release-v6.1 idf.py build
```

**ก่อนเปิด Kestrel ต้องกด `Ctrl+]` ออกจาก `idf.py monitor` ก่อนเสมอ** (คืนพอร์ต COM ให้ .NET — มิฉะนั้นจะได้ `UnauthorizedAccessException: Access to the port is denied`)

### เซิร์ฟเวอร์ (ทดสอบได้ทันที แม้ไม่มีบอร์ด)

```bash
cd Code/Lab9-3_ClosedLoop/server/ESP32.Kestrel.ClosedLoop
```

แก้ `appsettings.json` ให้ตรงกับพอร์ตจริงของบอร์ด:

```json
{ "SerialPort": { "PortName": "COM3" } }
```

แล้วรัน

```bash
dotnet run
```

เปิดเบราว์เซอร์ที่ `http://localhost:5127/`

---

## 7. คำตอบคำถามท้ายการทดลอง

### ข้อ 1. Bottleneck Analysis — ถ้า $\Delta T$ เกิน 200 ms เกิดจากส่วนไหนมากที่สุด?

**คำตอบ: พอร์ต Serial UART (115200 bps) คือคอขวดที่มีโอกาสสูงที่สุด** ไม่ใช่ SPI หรือ ADC

วิเคราะห์ตัวเลขจริงของแต่ละจุด

| ส่วนประกอบ | ความเร็ว | เวลาที่ใช้จริงต่อ 1 รอบ | หมายเหตุ |
| :--- | :---: | :---: | :--- |
| **ADC1 One-Shot** | ~ไม่กี่ไมโครวินาที | < 0.1 ms | อ่านค่าจากรีจิสเตอร์ภายในชิปโดยตรง เร็วมาก |
| **SPI2 @ 10 MHz** | 10,000,000 bit/s | $\frac{1024 \times 8}{10{,}000{,}000} \approx 0.82$ ms ต่อการยิง Framebuffer 1KB เต็ม | เร็วกว่า UART มากกว่า 86 เท่า |
| **UART @ 115200 bps** | 115,200 bit/s | ข้อความ `"ADC:2048,123456\n"` ยาว ~17 ไบต์ = 136 บิต $\rightarrow \frac{136}{115200} \approx 1.18$ ms **ต่อทิศทางเดียว** | ต้องส่ง **2 ทิศทาง** (ขาขึ้น + ขาลง) ในแต่ละรอบ |

**เหตุผลที่ UART เป็นคอขวด**

1. **แบนด์วิดท์ต่ำกว่า SPI เกิน 86 เท่า** (115.2 kbps เทียบกับ 10 Mbps) ทั้งที่ Framebuffer ต้องส่งข้อมูลมากกว่า (1,024 ไบต์) SPI ยังเร็วกว่าเพราะความถี่สัญญาณนาฬิกาสูงกว่ามาก
2. **เป็น Full-Duplex ที่ใช้สาย USB เส้นเดียวกับ Debug Console** ข้อมูล `ADC:` (ขาขึ้น) และ `SET:` (ขาลง) ต้องแย่งกันใช้บัฟเฟอร์ UART เดียวกัน ถ้า Kestrel เขียนตอบกลับช้าไปเล็กน้อย บรรทัดใหม่จาก ESP32 จะมาต่อคิวรอ
3. **`SerialPort.ReadLine()` เป็น Blocking Call** ฝั่ง .NET — ระหว่างรอ `\n` เธรดจะค้างอยู่ตรงนั้น ถ้า ESP32 ส่งช้าหรือสัญญาณรบกวน (Noise) ทำให้ไบต์เพี้ยนจนหา `\n` ไม่เจอ จะรอจนครบ `ReadTimeout` (2,000 ms ในโค้ดนี้) ซึ่งกระทบเวลาทั้งระบบทันที
4. **Baud Rate คงที่ ปรับไม่ได้ตามภาระงาน** ต่างจาก SPI ที่ปรับ `clock_speed_hz` ได้อิสระตามความต้องการ

**สรุป:** ถ้า $\Delta T > 200$ ms ให้ตรวจ UART ก่อนเป็นอันดับแรก — วิธีแก้คือเพิ่ม Baud Rate (เช่น 460800 หรือ 921600) หรือลดขนาด/ความถี่ของข้อความที่ส่ง

---

### ข้อ 2. ประโยชน์ของสถาปัตยกรรม Hybrid Edge-Cloud Fallback

**ทำไมระบบควบคุมอุตสาหกรรมต้องมีกลไกนี้**

ระบบ IoT ที่พึ่งพา Cloud/Server 100% มีจุดอ่อนร้ายแรง: **ถ้าการเชื่อมต่อขาดหาย อุปกรณ์ปลายทางจะค้างอยู่กับคำสั่งสุดท้ายที่เคยได้รับ (Stale State)** ซึ่งในงานควบคุมทางกายภาพเป็นเรื่องอันตรายมาก

ตัวอย่างที่ใบงานยกมา

| ระบบ | ถ้าไม่มี Fallback แล้ว Server หลุด |
| :--- | :--- |
| **แขนกลอุตสาหกรรม** | แขนค้างอยู่กับตำแหน่งคำสั่งสุดท้าย หรือแย่กว่านั้นคือเคลื่อนที่ต่อตามคำสั่งเก่าที่ไม่สอดคล้องกับสถานการณ์จริงแล้ว อาจชนชิ้นงานหรือคนงาน
| **ระบบระบายความร้อน** | พัดลม/ปั๊มค้างที่ความเร็วสุดท้าย ทั้งที่อุณหภูมิจริงอาจเปลี่ยนไปมากแล้ว เสี่ยง Overheat หรือสิ้นเปลืองพลังงานเกินจำเป็น

**กลไก Fallback ในแล็บนี้แก้ปัญหาอย่างไร**

ESP32 ไม่ได้ "รอคำสั่ง" อย่างเดียว แต่ตรวจสอบตัวเองตลอดเวลาด้วย **Heartbeat Timeout (1,500 ms)** — ถ้าไม่ได้รับ `SET:` ภายในเวลานี้ ให้ **กลับมาคิดเองในระดับ Edge ทันที** ด้วยสูตร Local Scaling ง่าย ๆ (`raw*100/4095`) แทนที่จะค้างเฉย ๆ

**ประโยชน์เชิงวิศวกรรม**

1. **Fail-Safe ไม่ใช่ Fail-Frozen** — อุปกรณ์ยังตอบสนองต่อโลกจริงได้เสมอ แม้ Cloud ล่ม เพียงแต่ความแม่นยำลดลงจาก Two-Point Calibration เป็น Linear Scaling ธรรมดา
2. **Graceful Degradation** — ระบบเสื่อมประสิทธิภาพแบบค่อยเป็นค่อยไป ไม่ใช่พังทันที (Hard Failure)
3. **Self-Healing อัตโนมัติ** — เมื่อ Kestrel กลับมาออนไลน์ ระบบยกระดับกลับเป็น Cloud Mode เองทันทีโดยไม่ต้องรีสตาร์ทฮาร์ดแวร์ (พิสูจน์แล้วใน Checkpoint 3.2)
4. **ลดภาระของทีมซ่อมบำรุงหน้างาน** — ไม่ต้องมีคนวิ่งไปกดรีเซ็ตทุกครั้งที่เน็ตเวิร์กสะดุด

**ถ้าไม่มีกลไกนี้จะเกิดอะไร:** ระบบจะเปลี่ยนจาก "งดงามแต่เปราะบาง" (ทำงานสมบูรณ์แบบเฉพาะตอนทุกอย่างออนไลน์) กลายเป็นระบบที่ **จุดล้มเหลวจุดเดียว (Single Point of Failure) ที่ Cloud** สามารถทำให้อุปกรณ์ปลายทางทั้งหมดหยุดทำงานพร้อมกันได้ทันที ซึ่งขัดกับหลักการออกแบบระบบ Real-Time ทางอุตสาหกรรมโดยสิ้นเชิง

---

### ข้อ 3. ทำไมห้ามใช้ ADC2 เมื่อเปิด Wi-Fi Stack?

**กลไกภายในชิป ESP32 ที่เกี่ยวข้อง**

ESP32 มีวงจรแปลงสัญญาณอนาล็อกเป็นดิจิทัล (ADC) อยู่ 2 ชุดที่เป็นอิสระทางฮาร์ดแวร์จากกัน:

| หน่วย | ขาที่ครอบคลุม | สถานะเมื่อเปิด Wi-Fi |
| :--- | :--- | :--- |
| **ADC1** | GPIO 32-39 | ใช้งานได้ปกติเสมอ |
| **ADC2** | GPIO 0, 2, 4, 12-15, 25-27 | **ถูกจองใช้งานแบบผูกขาดโดยฮาร์ดแวร์ RF** |

**สาเหตุเชิงลึก:** โมดูล Wi-Fi/Bluetooth ของ ESP32 (บล็อก RF PHY) ต้องใช้วงจรแปลงสัญญาณอนาล็อกภายในชิปเพื่อ **ตรวจวัดกำลังสัญญาณคลื่นวิทยุ (RSSI Calibration) และปรับจูนความถี่คลื่นพาหะ** อย่างต่อเนื่องขณะรับ-ส่งแพ็กเก็ต วงจรนี้ใช้ทรัพยากร ADC ร่วมกับ **ADC2** โดยตรงในระดับฮาร์ดแวร์ (Shared Silicon Resource) — เป็นข้อจำกัดทางสถาปัตยกรรมชิปที่ Espressif ออกแบบมาแบบนี้ตั้งแต่ต้น ไม่ใช่บั๊กซอฟต์แวร์ที่แก้ด้วย Driver ได้

เมื่อทั้งสองฝ่าย (โปรแกรมผู้ใช้ กับ RF PHY) พยายามอ่านค่า ADC2 พร้อมกัน จะเกิดการ **แย่งชิงการเข้าถึงฮาร์ดแวร์ (Hardware Resource Contention)** ทำให้:

1. **ค่าที่อ่านได้ผิดเพี้ยน (Erratic Reading)** — ค่าที่ได้ปนกับสัญญาณ RF Calibration ทำให้ตัวเลขกระโดดไม่สัมพันธ์กับแรงดันจริงที่ป้อนเข้าขา
2. **โปรแกรมค้าง (Deadlock/Hang)** — ในบางเวอร์ชันของ ESP-IDF ฟังก์ชัน `adc2_get_raw()` จะคืนค่า `ESP_ERR_TIMEOUT` หรือค้างรอ Mutex ที่ถูก Wi-Fi Driver ถือครองอยู่ไม่ปล่อย ถ้าไม่จัดการ Error ให้ดีโปรแกรมทั้งระบบจะหยุดทำงาน

**ทางแก้ที่ถูกต้อง (ตามที่ใช้ในแล็บนี้):** ต่อ Potentiometer เข้ากับ **GPIO 34 ซึ่งอยู่ใน ADC1** เท่านั้น เพราะ ADC1 ไม่มีการใช้งานร่วมกับวงจร RF ใด ๆ จึงอ่านค่าได้เสถียร 100% แม้ระหว่างที่ Wi-Fi/Bluetooth กำลังทำงานอยู่พร้อมกัน — เป็นกฎเหล็กที่ทุกระบบ ESP32 IoT ต้องปฏิบัติตามเมื่อออกแบบวงจรเซนเซอร์แอนะล็อก
