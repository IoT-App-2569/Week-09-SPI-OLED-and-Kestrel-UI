### กิจกรรมนิติวิทยาศาสตร์ 2.1  ทดสอบเรียกใช้งาน API ครบทั้ง 3 รูปแบบ

ให้นักศึกษาเปิด **PowerShell** หรือ **Terminal** (แยกอีกหน้าต่างหนึ่ง โดยปล่อยให้หน้าต่าง `dotnet run` ทำงานอยู่) แล้วทดสอบตามลำดับ:

#### 1. ตรวจสอบ Telemetry ปัจจุบัน (HTTP GET)

**คำสั่งด้วย `curl.exe`**
```powershell
curl.exe -i -X GET http://localhost:5117/api/telemetry
```
*(หมายเหตุ: เปลี่ยน `5117` เป็นหมายเลขพอร์ตที่หน้าจอ `dotnet run` ของตนเองระบุไว้ เช่น `5000` หรือ `5117`)*

**หรือคำสั่งด้วย PowerShell Native (`Invoke-RestMethod`)**
```powershell
Invoke-RestMethod -Uri http://localhost:5117/api/telemetry -Method Get
```

**ตัวอย่าง Raw Response ที่ต้องสังเกต**
```http
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Date: Sun, 13 Sep 2026 ...
Server: Kestrel

{"raw":2048,"calibrated":50.0,"unit":"%","displayMsg":"SYSTEM READY","timestamp":"2026-09-13T..."}
```
<img width="765" height="335" alt="image" src="https://github.com/user-attachments/assets/b4a212c0-dda4-497a-a1ae-9f9d6847cc85" />

---

#### 2. ทำการ Calibrate เซนเซอร์ใหม่ (HTTP POST พร้อม JSON Body)
จำลองการตั้งค่าช่วงแอนะล็อกดิบ 200 ถึง 3800 และสเกลเป็น 0 ถึง 1000 RPM

**คำสั่งด้วย `curl.exe`**
```powershell
curl.exe -i -X POST http://localhost:5117/api/potentiometer/calibrate `
  -H "Content-Type: application/json" `
  -d '{"rawMin": 200, "rawMax": 3800, "scaleMin": 0, "scaleMax": 1000, "unit": "RPM"}'
```

**หรือคำสั่งด้วย PowerShell Native (`Invoke-RestMethod`)**
```powershell
Invoke-RestMethod -Uri http://localhost:5078/api/potentiometer/calibrate -Method Post -ContentType "application/json" -Body '{"rawMin": 200, "rawMax": 3800, "scaleMin": 0, "scaleMax": 1000, "unit": "RPM"}'
```

**ตัวอย่าง Raw Response ที่ได้รับ**
```http
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Server: Kestrel

{"status":"success","settings":{"rawMin":200,"rawMax":3800,"scaleMin":0,"scaleMax":1000,"unit":"RPM"}}
```
<img width="815" height="220" alt="image" src="https://github.com/user-attachments/assets/6746cb1f-05e7-401e-8fa9-47b71acab62a" />

---

#### 3. ส่งข้อความใหม่ไปแสดงบนหน้าจอ OLED (HTTP POST)

**คำสั่งด้วย `curl.exe`**
```powershell
curl.exe -i -X POST http://localhost:5117/api/oled/message `
  -H "Content-Type: application/json" `
  -d '{"message":"Hello OLED"}'
```

**หรือคำสั่งด้วย PowerShell Native (`Invoke-RestMethod`)**
```powershell
Invoke-RestMethod -Uri http://localhost:5117/api/oled/message -Method Post -ContentType "application/json" -Body '{"message":"Hello OLED"}'
```

**ตัวอย่าง Raw Response ที่ได้รับ**
```http
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Server: Kestrel

{"status":"success","current":"Hello OLED"}
```
<img width="877" height="207" alt="image" src="https://github.com/user-attachments/assets/d692e722-3bd3-42f3-b838-28d983be0f06" />

---

> [!WARNING]
> **คลินิกตรวจบั๊กการใช้คำสั่ง (Forensic Troubleshooting Box):**
> * **เกิดข้อผิดพลาด `415 Unsupported Media Type`:**  
>   เกิดจากลืมใส่ `-H "Content-Type: application/json"` ทำให้ Kestrel ไม่ยอมรับข้อมูล Body
> * **เกิดข้อผิดพลาด `ParserError: Unexpected token '}'`:**  
>   เกิดจากการใส่เครื่องหมาย Double-Quote ซ้อนกันใน PowerShell ให้แก้ไขโดยใช้ **Single-Quote `'{"message":"..."}'`** ครอบข้อความ JSON เสมอ!
> * **เกิดข้อผิดพลาด `404 Not Found`:**  
>   เกิดจากพิมพ์ URL ผิด เช่น พิมพ์ `/api/oled/` ตกคำว่า `message`

---

### กิจกรรมนิติวิทยาศาสตร์ 2.2: Fault Injection & Vulnerability Probe (การจงใจฉีดข้อมูลวิกฤต)

ในฐานะวิศวกร เราต้องทดสอบว่าระบบจะรับมือกับข้อมูลผิดพลาดได้อย่างปลอดภัยหรือไม่

1. **ทดสอบป้อนค่าสเกลผิดตรรกะ (Span Point น้อยกว่า Zero Point)**
   ```powershell
   curl.exe -i -X POST http://localhost:5078/api/potentiometer/calibrate `
     -H "Content-Type: application/json" `
     -d '{"rawMin": 4000, "rawMax": 1000, "scaleMin": 0, "scaleMax": 100, "unit": "%"}'
   ```
   * **ผลที่คาดหวัง:** เซิร์ฟเวอร์ต้องตอบกลับด้วย **`400 Bad Request`** พร้อมข้อความเตือน `"RawMax ต้องมีค่ามากกว่า RawMin เสมอ!"` โดยที่เซิร์ฟเวอร์ Kestrel **ไม่ล่ม (No Server Crash)**!
<img width="1211" height="297" alt="image" src="https://github.com/user-attachments/assets/a33cd965-9817-4d50-a2d2-f8ae93783417" />

2. **ทดสอบส่งข้อความว่างเปล่า:**
   ```powershell
   curl.exe -i -X POST http://localhost:5078/api/oled/message `
     -H "Content-Type: application/json" `
     -d '{"message":""}'
   ```
   * **ผลที่คาดหวัง:** ได้รับ **`400 Bad Request`** แจ้งว่าข้อความต้องไม่ว่างเปล่า
<img width="792" height="185" alt="image" src="https://github.com/user-attachments/assets/c18c2636-d392-4dad-be73-0e13020d5876" />


---

## 5. คำถามท้ายการทดลองเพื่อการประเมินผล

### 1. เหตุใดการคำนวณสเกลเซนเซอร์จึงควรทำที่ฝั่ง Kestrel Server แทนที่จะคำนวณบนไมโครคอนโทรลเลอร์ ESP32 ตั้งแต่แรก?
**คำตอบ:**
1. **ลดภาระทรัพยากรของไมโครคอนโทรลเลอร์ (Resource Optimization):** ESP32 มีทรัพยากร CPU และ RAM จำกัด การให้ ESP32 ทำหน้าที่เพียงอ่านและสตรีมค่าดิบ (Raw ADC Data) จะช่วยลดภาระการคำนวณคณิตศาสตร์จุดทศนิยม (Floating-point Operation) บน Edge Device
2. **ความยืดหยุ่นในการปรับแต่งพารามิเตอร์ (Centralized & Dynamic Calibration):** หากต้องเปลี่ยนช่วงสเกล (เช่น จาก 0-100% เป็น 0-1000 RPM) หรือเปลี่ยนค่า Zero/Span สามารถทำได้ทันทีบน Kestrel Server ผ่าน Web UI/API โดยไม่จำเป็นต้องแก้ไขโค้ดและทำการ Re-flash Firmware ใหม่ไปยัง ESP32
3. **การรักษาข้อมูลดิบตั้งต้นและการประมวลผลขั้นสูง (Data Integrity & Precision):** การส่งค่า Raw ADC มายัง Server ช่วยให้สามารถบันทึกประวัติข้อมูลดิบเพื่อนำมาวิเคราะห์หรือ Re-calibrate ย้อนหลังได้ และฝั่ง Server มีกำลังประมวลผลสูงกว่า ทำให้รองรับอัลกอริทึมการกรองสัญญาณหรือปรับเทียบที่ซับซ้อนได้ดีกว่า

---

### 2. จากการทำ HTTP Forensics หากไม่มีการตรวจสอบเงื่อนไข `RawMax <= RawMin` ในโค้ด จะเกิด Exception ชนิดใดขึ้นในภาษา C# และส่งผลต่อการทำงานของเซิร์ฟเวอร์อย่างไร?
**คำตอบ:**
1. **Exception / พฤติกรรมที่เกิดขึ้น:** 
   - หาก `RawMax == RawMin` ตัวหาร `(RawMax - RawMin)` จะมีค่าเท่ากับ `0` ใน C# การหารเลขทศนิยม (`double`) ด้วย `0` จะได้ผลลัพธ์เป็น `double.PositiveInfinity` หรือ `double.NaN` (Not a Number) ซึ่งเมื่อนำไปแปลงเป็น JSON จะส่งผลให้เกิด **`JsonException`** (Serialization Error) เนื่องจากมาตรฐาน JSON ไม่รองรับค่า Infinity หรือ NaN
   - หากเป็นเลขจำนวนเต็ม (`int`) หรือมีการโยนข้อผิดพลาด จะเกิด **`DivideByZeroException`** หรือ **`ArgumentException`**
2. **ผลกระทบต่อเซิร์ฟเวอร์ (Server Impact):**
   - หากเกิด Unhandled Exception ขึ้น Kestrel จะตอบกลับไคลเอ็นต์ด้วยรหัสสถานะ **`500 Internal Server Error`**
   - ส่งผลให้ระบบตอบสนองอย่างไม่ถูกต้อง ข้อมูลที่ส่งกลับไคลเอ็นต์ล้มเหลว และอาจทำให้การประมวลผล Telemetry หรือบริการ API ขัดข้องได้ หากไม่มีการจัดการ Exception Handling (Input Validation) ไว้ล่วงหน้า

---

### 3. อธิบายสาเหตุทางเทคนิคว่าทำไมคำขอ HTTP POST ที่ไม่มี Header `Content-Type: application/json` จึงถูกปฏิเสธด้วยรหัสสถานะ `415 Unsupported Media Type`?
**คำตอบ:**
1. **การทำงานของ Content Negotiation & Model Binding:** ใน ASP.NET Core Minimal API ตัว Kestrel Server ใช้ระบบ Request Body Binding ในการแปลงข้อมูล Payload จาก HTTP Request Body ไปเป็น C# Object (เช่น `CalibrationSettings` หรือ `DisplayMessageRequest`) โดยระบบต้องพึ่งพา Header `Content-Type` เพื่อเลือก `IInputFormatter` (เช่น `System.Text.Json`) ที่ถูกต้องมาใช้ถอดรหัสข้อมูล
2. **การปฏิบัติตามมาตรฐาน HTTP Spec (RFC 9110):** เมื่อ Client ส่งคำขอแบบ POST ที่มี Request Body แต่ลืมระบุ `Content-Type: application/json` หรือระบุประเภทที่ไม่ตรง Kestrel จะไม่สามารถทราบฟอร์แมตข้อมูลได้ จึงปฏิเสธคำขอทันทีเพื่อความปลอดภัยและป้องกันการอ่านข้อมูลผิดรูปแบบ
3. **การตอบกลับด้วยรหัส 415:** รหัส **`415 Unsupported Media Type`** เป็นรหัสมาตรฐาน HTTP ที่แจ้งให้ไคลเอ็นต์ทราบว่า เซิร์ฟเวอร์ไม่รองรับฟอร์แมตของ Payload ที่ส่งมาใน Request และไคลเอ็นต์ต้องระบุ Header `Content-Type: application/json` ให้ถูกต้องก่อนส่งคำขอใหม่



