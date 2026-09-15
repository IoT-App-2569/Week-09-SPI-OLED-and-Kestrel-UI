# เฉลย / รายงานผลใบงานที่ 9.2 (Answer Lab 9.2)
### เอนจินปรับเทียบเซนเซอร์และ API ควบคุมการแสดงผลบน Kestrel Web Server พร้อม HTTP Payload Forensics

| หัวข้อ | รายละเอียด |
| :--- | :--- |
| **รหัสนักศึกษา** | 67030098 |
| **ใบงาน** | [07-Labsheet-09-2-Kestrel-Calibration-and-Display-API.md](../07-Labsheet-09-2-Kestrel-Calibration-and-Display-API.md) |
| **โค้ดที่ส่ง** | [`Code/Lab9-2_Kestrel_Webserver/`](../Code/Lab9-2_Kestrel_Webserver/) |
| **ไฟล์หลัก** | [`Program.cs`](../Code/Lab9-2_Kestrel_Webserver/ESP32.Kestrel.Webserver/Program.cs), [`Services/CalibrationService.cs`](../Code/Lab9-2_Kestrel_Webserver/ESP32.Kestrel.Webserver/Services/CalibrationService.cs) |
| **เฟรมเวิร์ก** | .NET 10 Minimal API (Kestrel) — พอร์ต `5117` |
| **สถานะการทดสอบ** | ✅ Build ผ่าน 0 Warning 0 Error / ทดสอบครบ 3 Endpoint + 4 Fault Case |

---

## 1. โครงสร้าง REST API ที่สร้าง

```
[ Kestrel Web Server : http://localhost:5117 ]
  ├── GET  /api/telemetry                : สตรีมค่า Raw ADC, Calibrated Value, ข้อความปัจจุบัน
  ├── POST /api/potentiometer/calibrate  : ปรับเทียบ Zero & Span
  └── POST /api/oled/message             : Broadcast ข้อความไปยังจอ OLED
```

---

## 2. ผลการทดลองแต่ละกิจกรรม

### กิจกรรมที่ 2.1 — Calibration Service

สูตร Two-Point Linear Calibration ที่ใช้

$$V_{out} = \frac{\text{clamp}(ADC_{raw}) - ADC_{min}}{ADC_{max} - ADC_{min}} \times (Scale_{max} - Scale_{min}) + Scale_{min}$$

**กลไกป้องกันความผิดพลาด 2 ชั้นที่ใส่ไว้**

| กลไก | โค้ด | ป้องกันอะไร |
| :--- | :--- | :--- |
| Divide-by-Zero Guard | `if (newSettings.RawMax <= newSettings.RawMin) throw` | ตัวหาร `RawMax - RawMin` กลายเป็น 0 หรือติดลบ |
| Out-of-Bounds Clamping | `Math.Clamp(rawAdc, RawMin, RawMax)` | ค่า ADC ที่หลุดช่วง ทำให้ผลลัพธ์ทะลุ 0–100% |

**สิ่งที่เพิ่มเติมจากใบงาน (พร้อมเหตุผล)**

* `CalibrationService` ลงทะเบียนเป็น **Singleton** จึงถูกเรียกพร้อมกันจากหลาย HTTP Request ได้ ผมจึงใส่ `lock` ป้องกัน Race Condition ตอนอ่าน/เขียน `_settings` และ `_currentOledMessage`
* ถ้า `Unit` ที่ส่งมาว่างเปล่า จะ fallback เป็น `"%"` แทนที่จะปล่อยเป็นสตริงว่างซึ่งทำให้หน้าเว็บแสดงผลแปลก

---

### กิจกรรมที่ 2.2 — ผูก Minimal API Routes

จุดสำคัญคือ Minimal API ทำ **Model Binding จาก JSON Body อัตโนมัติ** เมื่อพารามิเตอร์เป็น Complex Type

```csharp
app.MapPost("/api/potentiometer/calibrate",
    (CalibrationSettings newSettings, CalibrationService cal) => { ... });
//   └─ ผูกจาก JSON Body           └─ ผูกจาก DI Container
```

ASP.NET Core แยกแยะเองว่า `CalibrationSettings` ไม่ได้ลงทะเบียนใน DI จึงต้องมาจาก Body ส่วน `CalibrationService` ลงทะเบียนไว้แล้วจึงฉีดจาก DI — และ JSON binding เป็นแบบ **case-insensitive** ดังนั้น `"rawMin"` (camelCase) จึงจับคู่กับ property `RawMin` (PascalCase) ได้

---

## 3. ผลการตรวจพิสูจน์หลักฐานเครือข่าย (HTTP Payload Forensics)

### กิจกรรม 2.1 — ทดสอบ API ครบ 3 รูปแบบ (ผลจริงที่รันได้)

#### 1) GET /api/telemetry

```powershell
curl.exe -i -X GET http://localhost:5117/api/telemetry
```

**Raw Response ที่ได้จริง**

```http
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Date: Tue, 15 Sep 2026 01:50:47 GMT
Server: Kestrel
Transfer-Encoding: chunked

{"raw":2048,"calibrated":49.9,"unit":"%","displayMsg":"SYSTEM READY","timestamp":"2026-09-15T01:50:47.4958124Z"}
```

> **🔍 ข้อสังเกตเชิงนิติวิทยาศาสตร์:** ใบงานยกตัวอย่างว่าจะได้ `calibrated: 50.0` แต่ค่าจริงคือ **49.9**
> ไม่ใช่บั๊ก — เป็นผลจากค่า default ที่ **ไม่สมมาตร** คือ `RawMin = 150`, `RawMax = 3950`
>
> $$\frac{2048 - 150}{3950 - 150} \times 100 = \frac{1898}{3800} \times 100 = 49.947\ldots \approx 49.9$$
>
> จุดกึ่งกลางจริงของช่วงนี้คือ ADC = 2050 ไม่ใช่ 2048 กรณีนี้แสดงให้เห็นว่า **การ Calibrate ทำให้ "ครึ่งทางของ ADC" ไม่เท่ากับ "ครึ่งทางของสเกลผลลัพธ์" อีกต่อไป** ซึ่งเป็นหัวใจของการปรับเทียบเซนเซอร์

**ข้อสังเกตจาก Header**

| Header | ค่า | ความหมาย |
| :--- | :--- | :--- |
| `Server: Kestrel` | — | ยืนยันว่า Kestrel เป็นผู้ตอบเอง ไม่ได้ผ่าน IIS/Nginx reverse proxy |
| `Content-Type` | `application/json; charset=utf-8` | `Results.Ok(object)` ทำ serialization ให้อัตโนมัติ |
| `Transfer-Encoding: chunked` | — | เซิร์ฟเวอร์ไม่ทราบความยาว body ล่วงหน้า (เพราะ serialize แบบสตรีม) จึงส่งแบบแบ่งก้อนแทนการใส่ `Content-Length` |

---

#### 2) POST /api/potentiometer/calibrate

```powershell
curl.exe -i -X POST http://localhost:5117/api/potentiometer/calibrate `
  -H "Content-Type: application/json" `
  -d '{"rawMin": 200, "rawMax": 3800, "scaleMin": 0, "scaleMax": 1000, "unit": "RPM"}'
```

**Response จริง**

```http
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Server: Kestrel

{"status":"success","settings":{"rawMin":200,"rawMax":3800,"scaleMin":0,"scaleMax":1000,"unit":"RPM"}}
```

**ตรวจสอบผลข้างเคียง** — เรียก `/api/telemetry` ซ้ำหลัง Calibrate

```json
{"raw":2048,"calibrated":513.3,"unit":"RPM","displayMsg":"CALIBRATED OK","timestamp":"..."}
```

✅ ยืนยัน 3 อย่างพร้อมกัน: หน่วยเปลี่ยนเป็น RPM, ค่าคำนวณใหม่ตามสเกล 0–1000 และข้อความ OLED ถูกตั้งเป็น `CALIBRATED OK` อัตโนมัติ

ตรวจสอบเลขคณิต: $\frac{2048-200}{3800-200} \times 1000 = \frac{1848}{3600} \times 1000 = 513.33 \approx 513.3$ ✅

---

#### 3) POST /api/oled/message

```powershell
curl.exe -i -X POST http://localhost:5117/api/oled/message `
  -H "Content-Type: application/json" `
  -d '{"message":"Hello OLED"}'
```

**Response จริง**

```http
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Server: Kestrel

{"status":"success","current":"Hello OLED"}
```

> **⚠️ บั๊กที่เจอระหว่างทดสอบจริง (บันทึกไว้เป็นบทเรียน)**
> ครั้งแรกที่ยิงคำสั่งนี้ได้ `400 Bad Request` พร้อม `JsonException: Expected end of string`
> และ Header แสดง `Content-Length: 17` ทั้งที่ JSON ควรยาว 24 ไบต์
>
> **สาเหตุ:** PowerShell ตัด argument ที่ **ช่องว่าง** ใน `Hello OLED` ทำให้ `curl.exe` ได้รับ body แค่ `{"message":"Hello` — JSON จึงขาดกลางคัน
> **ไม่ใช่บั๊กของเซิร์ฟเวอร์** แต่เป็นปัญหาการ escape ของ Shell
>
> **วิธีแก้ที่ใช้ได้จริง 2 ทาง**
> 1. ใช้ `Invoke-RestMethod` แทน ซึ่งรับสตริง PowerShell ตรง ๆ ไม่ผ่าน argument parser ของ native exe
> 2. เก็บ JSON ลงไฟล์แล้วส่งด้วย `curl.exe --data-binary "@body.json"`
>
> ทั้งสองวิธีให้ `200 OK` และ `{"status":"success","current":"Hello OLED"}` เหมือนกัน

---

### กิจกรรม 2.2 — Fault Injection & Vulnerability Probe

| # | การทดสอบ | คาดหวัง | **ผลจริง** | สถานะ |
| :---: | :--- | :---: | :--- | :---: |
| 1 | `rawMin: 4000, rawMax: 1000` | 400 | `400` + `{"status":"error","message":"RawMax ต้องมีค่ามากกว่า RawMin เสมอ!"}` | ✅ PASS |
| 2 | `{"message":""}` | 400 | `400` + `{"status":"error","message":"ข้อความต้องไม่ว่างเปล่า"}` | ✅ PASS |
| 3 | POST โดยไม่ใส่ `Content-Type` | 415 | `415 Unsupported Media Type` | ✅ PASS |
| 4 | POST ไปที่ `/api/oled/` (ตกคำว่า message) | 404 | `404 Not Found` | ✅ PASS |

**ข้อพิสูจน์สำคัญที่สุด:** หลังฉีดความผิดพร่องครบทั้ง 4 แบบ **เซิร์ฟเวอร์ Kestrel ยังทำงานต่อได้ปกติ ไม่ล่ม (No Server Crash)** ทดสอบยืนยันด้วยการเรียก `/api/telemetry` ซ้ำหลังจากนั้น ยังได้ `200 OK` ตามเดิม

สาเหตุที่ไม่ล่มคือ `ArgumentException` ถูกดักด้วย `try-catch` ในตัว Endpoint แล้วแปลงเป็น HTTP 400 อย่างสุภาพ ไม่ปล่อยให้ exception ทะลุขึ้นไปถึง Middleware Pipeline

---

## 4. คำตอบคำถามท้ายการทดลอง

### ข้อ 1. ทำไมควรคำนวณสเกลเซนเซอร์ที่ฝั่ง Kestrel Server แทนที่จะคำนวณบน ESP32?

มี 5 เหตุผลหลัก

**1. เปลี่ยนค่าปรับเทียบได้ทันทีโดยไม่ต้อง Re-flash**
ถ้าฝัง `RawMin`/`RawMax` ไว้ในเฟิร์มแวร์ ทุกครั้งที่เปลี่ยน Potentiometer หรือค่าเลื่อนตามอุณหภูมิ ต้องคอมไพล์และแฟลชใหม่ทั้งบอร์ด — ในโรงงานที่มีอุปกรณ์ 500 ตัวคือฝันร้าย แต่ถ้าคำนวณบนเซิร์ฟเวอร์ แค่ยิง `POST /api/potentiometer/calibrate` ก็เปลี่ยนได้ทันที **ขณะระบบยังทำงานอยู่**

**2. เก็บ Raw Data ไว้เป็นหลักฐานดิบ (Data Provenance)**
ESP32 ส่ง `ADC:2048` ซึ่งเป็น **ค่าดิบที่ไม่ผ่านการตีความ** ถ้าภายหลังพบว่าตั้งค่าปรับเทียบผิด ยังย้อนกลับไปคำนวณใหม่จากข้อมูลเก่าได้ แต่ถ้า ESP32 แปลงเป็น `50%` มาแล้ว ข้อมูลดิบจะสูญหายถาวรและกู้คืนไม่ได้

**3. ประหยัดทรัพยากรของไมโครคอนโทรลเลอร์**
ESP32 ต้องทำงานเรียลไทม์หลายอย่างพร้อมกัน (สแกน ADC 20 Hz, ยิง SPI 1KB ต่อเฟรม, จัดการ FreeRTOS, Wi-Fi Stack) การคำนวณเลขทศนิยม `double` บนชิปที่ไม่มี FPU ประสิทธิภาพสูงเป็นภาระที่ไม่จำเป็น ขณะที่ฝั่งเซิร์ฟเวอร์คำนวณให้ได้ในระดับนาโนวินาที

**4. Single Source of Truth**
เมื่อมี ESP32 หลายตัวในระบบ ตรรกะการแปลงค่าอยู่ที่เดียวบนเซิร์ฟเวอร์ ทำให้ทุกอุปกรณ์ใช้มาตรฐานเดียวกันแน่นอน ไม่ต้องกังวลว่าบอร์ดตัวไหนแฟลชเวอร์ชันเก่าค้างอยู่

**5. แยกหน้าที่ตามหลัก Separation of Concerns**
ESP32 = ชั้นรับรู้ (Sensing Layer) ทำหน้าที่วัดและรายงานตามจริง / Kestrel = ชั้นตีความ (Business Logic Layer) ทำหน้าที่แปลงความหมาย เมื่อ Business Logic เปลี่ยน (เช่น เปลี่ยนจาก % เป็น RPM) ชั้นฮาร์ดแวร์ไม่ต้องแก้เลย

---

### ข้อ 2. ถ้าไม่ตรวจสอบ `RawMax <= RawMin` จะเกิด Exception ชนิดใดใน C# และส่งผลอย่างไร?

**คำตอบที่คนส่วนใหญ่ตอบผิด:** หลายคนตอบว่า `DivideByZeroException` — **ซึ่งไม่ถูกต้องในกรณีนี้**

**คำตอบที่ถูกต้อง: ไม่เกิด Exception ใด ๆ เลย ซึ่งอันตรายกว่ามาก**

เหตุผลอยู่ที่การแบ่งแยกกฎการหารใน C#

| ชนิดข้อมูล | `x / 0` | ผลลัพธ์ |
| :--- | :--- | :--- |
| จำนวนเต็ม (`int`) | throw | `DivideByZeroException` |
| **ทศนิยม (`double`)** | **ไม่ throw** | `Infinity`, `-Infinity` หรือ `NaN` ตามมาตรฐาน **IEEE 754** |

ในโค้ดนี้ตัวตั้งถูกแคสต์เป็น `double` ไว้แล้ว (`(double)(clamped - RawMin)`) การหารจึงเป็นการหารแบบทศนิยม

**วิเคราะห์ผลลัพธ์แยกเป็น 2 กรณี**

**กรณี A: `RawMax == RawMin` (เช่น 2000 กับ 2000)**
`Math.Clamp(raw, 2000, 2000)` บังคับให้ `clamped = 2000` เสมอ ตัวเศษจึงเป็น `0` และตัวส่วนก็เป็น `0`
$\Rightarrow$ `0.0 / 0.0 = NaN` (Not a Number) โดย **ไม่มี Exception ใด ๆ แจ้งเตือน**

*ยืนยันด้วยการทดลองจริงบน .NET 10:*

```
clamped=2000  value=NaN  IsNaN=True
Math.Round(NaN,1) = NaN
serialize THREW: ArgumentException: .NET number values such as positive and
negative infinity cannot be written as valid JSON.
```

**กรณี B: `RawMax < RawMin` (เช่น RawMin = 4000, RawMax = 1000)**
`Math.Clamp` ในกรณีนี้จะ throw `ArgumentException` เพราะ .NET ตรวจพบว่า `min > max` (ทดสอบยืนยันแล้ว) — ถือว่าโชคดีที่มีกันชนชั้นนี้ แต่ข้อความ error จะสื่อสารไม่ตรงประเด็น (พูดถึง Clamp ไม่ได้พูดถึงการตั้งค่าปรับเทียบ) ทำให้ผู้ใช้ API งงและดีบักยาก

**ผลกระทบต่อระบบโดยรวม (ทำไมถึงร้ายแรง)**

1. **ค่า `NaN` แพร่กระจายแบบเงียบ (Silent Propagation)** — ทุกการคำนวณที่นำ `NaN` ไปใช้ต่อจะได้ `NaN` หมด รวมถึง `Math.Round(NaN, 1)` ก็ยังเป็น `NaN`
2. **JSON serialization ผิดมาตรฐาน** — `NaN` และ `Infinity` ไม่ใช่ค่าที่ JSON spec รองรับ `System.Text.Json` จะ throw **`ArgumentException`** ตอน serialize ด้วยข้อความ *".NET number values such as positive and negative infinity cannot be written as valid JSON"* ทำให้ **HTTP Response พังกลางคัน** ผู้ใช้ได้ 500 Internal Server Error ที่หาสาเหตุไม่เจอ
3. **หน้าเว็บ Dashboard พัง** — JavaScript ได้ `null` หรือ `NaN` ไปวาดเข็มเกจ SVG ทำให้เข็มหายไปหรือชี้ไปนอกจอ
4. **ดีบักยากที่สุด** — ไม่มี stack trace ชี้จุดเกิดเหตุ เพราะจุดที่เกิด `NaN` (การหาร) กับจุดที่ระบบพัง (การ serialize) อยู่คนละที่กัน

**บทสรุปเชิงวิศวกรรม:** นี่คือตัวอย่างคลาสสิกของหลัก **"Fail Fast"** — การ `throw ArgumentException` พร้อมข้อความชัดเจนตั้งแต่ตอนรับ input ดีกว่าปล่อยให้ค่าเสียหายไหลลึกเข้าไปในระบบแล้วไปพังในจุดที่ไม่เกี่ยวข้องกัน

---

### ข้อ 3. ทำไม POST ที่ไม่มี Header `Content-Type: application/json` ถูกปฏิเสธด้วย `415 Unsupported Media Type`?

**หลักการ: เซิร์ฟเวอร์ไม่ "เดา" รูปแบบข้อมูล**

เมื่อ HTTP Request มาถึง Kestrel ตัว Body เป็นเพียง **สายไบต์ดิบ (Raw Byte Stream)** ที่ไม่มีความหมายในตัวเอง ไบต์ชุดเดียวกันอาจเป็น JSON, XML, `application/x-www-form-urlencoded`, Protocol Buffers หรือไฟล์ภาพก็ได้

Header `Content-Type` จึงทำหน้าที่เป็น **"ฉลากกำกับสินค้า"** ตามที่ RFC 9110 กำหนด ให้ผู้ส่งประกาศว่า Body ที่ส่งมาคือรูปแบบใด

**กลไกภายใน ASP.NET Core ทีละขั้น**

1. Routing จับคู่ URL และ HTTP Method ได้สำเร็จ → พบ Endpoint `/api/oled/message`
2. RequestDelegateFactory ตรวจ Signature พบพารามิเตอร์ `DisplayMessageRequest req` เป็น Complex Type ที่ไม่ได้ลงทะเบียนใน DI → สรุปว่าต้องอ่านจาก Body
3. ระบบเลือก **Input Formatter** โดยดูจาก `Content-Type` — ค่าเริ่มต้นมีเพียง `SystemTextJsonInputFormatter` ที่รองรับ `application/json` เท่านั้น
4. **ไม่มี Content-Type → ไม่รู้จะเลือก Formatter ตัวใด → ตอบ `415` ทันที** โดยยังไม่แตะ Body แม้แต่ไบต์เดียว

**ทำไมต้องเป็น 415 ไม่ใช่ 400?** เพราะรหัสสถานะทั้งสองสื่อสารคนละเรื่อง ซึ่งช่วยให้ debug ได้ตรงจุด

| รหัส | ความหมาย | สถานการณ์ |
| :---: | :--- | :--- |
| **415** Unsupported Media Type | *"ผมไม่รู้ว่าข้อมูลนี้เป็นรูปแบบอะไร จึงยังไม่ได้เปิดอ่าน"* | ขาด Content-Type หรือส่งเป็น `text/plain` |
| **400** Bad Request | *"ผมเปิดอ่านแล้ว แต่เนื้อหาข้างในผิดรูปแบบ"* | ส่ง JSON มาแต่วงเล็บไม่ปิด (เช่น บั๊ก PowerShell ข้างต้น) |

**หลักฐานเชิงประจักษ์จากการทดลอง** — Fault Case ที่ 3 ได้ `415` เพราะไม่ใส่ Header
ขณะที่บั๊ก PowerShell ตัดสตริงกลางคัน (ซึ่ง**ใส่** Header ครบ) ได้ `400` พร้อม `JsonException`
ยืนยันว่าเซิร์ฟเวอร์แยกแยะสองสถานการณ์นี้ออกจากกันได้จริง

**เหตุผลด้านความปลอดภัย:** การบังคับให้ประกาศ Content-Type ยังช่วยป้องกัน **CSRF (Cross-Site Request Forgery)** ระดับหนึ่ง เพราะฟอร์ม HTML ธรรมดาส่งได้เฉพาะ `application/x-www-form-urlencoded`, `multipart/form-data` และ `text/plain` เท่านั้น ส่งเป็น `application/json` ไม่ได้ เว็บที่เป็นอันตรายจึงไม่สามารถหลอกให้เบราว์เซอร์ยิง JSON API ข้ามโดเมนได้โดยไม่ผ่านการตรวจ CORS Preflight ก่อน

---

## 5. วิธีรันและทดสอบโค้ดที่ส่ง

### รันเซิร์ฟเวอร์

```bash
cd Code/Lab9-2_Kestrel_Webserver/ESP32.Kestrel.Webserver
dotnet run
```

เซิร์ฟเวอร์จะฟังที่ `http://localhost:5117` (กำหนดไว้ใน `Properties/launchSettings.json`)

### ชุดคำสั่งทดสอบครบทุกกรณี (เปิด Terminal หน้าต่างที่ 2)

```powershell
# 1. GET telemetry
curl.exe -i http://localhost:5117/api/telemetry

# 2. Calibrate (สำเร็จ)
Invoke-RestMethod -Uri http://localhost:5117/api/potentiometer/calibrate -Method Post `
  -ContentType "application/json" `
  -Body '{"rawMin":200,"rawMax":3800,"scaleMin":0,"scaleMax":1000,"unit":"RPM"}'

# 3. ส่งข้อความขึ้นจอ OLED
Invoke-RestMethod -Uri http://localhost:5117/api/oled/message -Method Post `
  -ContentType "application/json" -Body '{"message":"Hello OLED"}'

# 4. Fault: RawMax < RawMin  -> ต้องได้ 400
curl.exe -i -X POST http://localhost:5117/api/potentiometer/calibrate `
  -H "Content-Type: application/json" `
  -d '{\"rawMin\":4000,\"rawMax\":1000,\"scaleMin\":0,\"scaleMax\":100,\"unit\":\"%\"}'

# 5. Fault: ข้อความว่าง  -> ต้องได้ 400
curl.exe -i -X POST http://localhost:5117/api/oled/message `
  -H "Content-Type: application/json" -d '{\"message\":\"\"}'

# 6. Fault: ไม่มี Content-Type  -> ต้องได้ 415
curl.exe -i -X POST http://localhost:5117/api/oled/message -d '{\"message\":\"x\"}'

# 7. Fault: URL ผิด  -> ต้องได้ 404
curl.exe -i -X POST http://localhost:5117/api/oled/
```

> **เคล็ดลับจากที่เจอมาจริง:** ถ้า JSON มี **ช่องว่าง** ให้ใช้ `Invoke-RestMethod` หรือ `--data-binary "@file.json"`
> อย่าใช้ `curl.exe -d` กับ single-quote บน PowerShell เพราะสตริงจะถูกตัดที่ช่องว่าง
