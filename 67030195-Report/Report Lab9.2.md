# ผลลัพธ์การทดลองของใบงานการทดลองที่ 9.2 (Lab 9.2)
### การพัฒนาเอนจินปรับเทียบเซนเซอร์และ API ควบคุมการแสดงผลบน Kestrel Web Server พร้อมการพิสูจน์หลักฐานเครือข่าย (HTTP Payload Forensics)
สังเกตุ url ที่ระบบแจ้งมา และให้เปิด web browser ไปที่นั่น
ต้องเห็นข้อความ "Hello World!"

<img width="1530" height="340" alt="สกรีนช็อต 2026-09-16 222032" src="https://github.com/user-attachments/assets/bb191594-09cb-46ec-a44c-d61718663909" />
   
### รูปscreenshot สังเกต Raw HTTP Response Headers

<img width="1535" height="813" alt="สกรีนช็อต 2026-09-16 223500" src="https://github.com/user-attachments/assets/6588ec2d-26b9-4b48-bc2f-dc3d001509de" />

---

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

<img width="872" height="210" alt="สกรีนช็อต 2026-09-16 225648" src="https://github.com/user-attachments/assets/86beace8-eadd-4f1e-90b2-485c241c79c5" />

---

#### 2. ทำการ Calibrate เซนเซอร์ใหม่ (HTTP POST พร้อม JSON Body)
จำลองการตั้งค่าช่วงแอนะล็อกดิบ 200 ถึง 3800 และสเกลเป็น 0 ถึง 1000 RPM

**หรือคำสั่งด้วย PowerShell Native (`Invoke-RestMethod`)**
```powershell
Invoke-RestMethod -Uri http://localhost:5117/api/potentiometer/calibrate -Method Post -ContentType "application/json" -Body '{"rawMin": 200, "rawMax": 3800, "scaleMin": 0, "scaleMax": 1000, "unit": "RPM"}'
```
<img width="1218" height="126" alt="สกรีนช็อต 2026-09-16 230041" src="https://github.com/user-attachments/assets/00b165de-562c-4579-834d-9b8deb38c11a" /> 
<img width="1206" height="175" alt="สกรีนช็อต 2026-09-16 230300" src="https://github.com/user-attachments/assets/cc07871d-97ef-428c-9255-d0df23230b27" />

---

#### 3. ส่งข้อความใหม่ไปแสดงบนหน้าจอ OLED (HTTP POST)
**ใช้คำสั่งด้วย PowerShell Native (`Invoke-RestMethod`)**
```powershell
Invoke-RestMethod -Uri http://localhost:5117/api/oled/message -Method Post -ContentType "application/json" -Body '{"message":"Hello OLED"}'
```
<img width="1182" height="157" alt="สกรีนช็อต 2026-09-16 230120" src="https://github.com/user-attachments/assets/633e8137-140c-40d9-8334-bb0a999eb0e9" />

---
### กิจกรรมนิติวิทยาศาสตร์ 2.2: Fault Injection & Vulnerability Probe (การจงใจฉีดข้อมูลวิกฤต)
ในฐานะวิศวกร เราต้องทดสอบว่าระบบจะรับมือกับข้อมูลผิดพลาดได้อย่างปลอดภัยหรือไม่

1. **ทดสอบป้อนค่าสเกลผิดตรรกะ (Span Point น้อยกว่า Zero Point)**
   ```powershell
   curl.exe -i -X POST http://localhost:5117/api/potentiometer/calibrate `
     -H "Content-Type: application/json" `
     -d '{"rawMin": 4000, "rawMax": 1000, "scaleMin": 0, "scaleMax": 100, "unit": "%"}'
   ```
   * **ผลที่คาดหวัง:** เซิร์ฟเวอร์ต้องตอบกลับด้วย **`400 Bad Request`** พร้อมข้อความเตือน `"RawMax ต้องมีค่ามากกว่า RawMin เสมอ!"` โดยที่เซิร์ฟเวอร์ Kestrel **ไม่ล่ม (No Server Crash)**!
   * **รูปภาพผลลัพธ์** 
  ![Uploading สกรีนช็อต 2026-09-16 230300.png…]()

2. **ทดสอบส่งข้อความว่างเปล่า:**
   ```powershell
   curl.exe -i -X POST http://localhost:5117/api/oled/message `
     -H "Content-Type: application/json" `
     -d '{"message":""}'
   ```
   * **ผลที่คาดหวัง:** ได้รับ **`400 Bad Request`** แจ้งว่าข้อความต้องไม่ว่างเปล่า
   * **รูปภาพผลลัพธ์**
  <img width="1226" height="202" alt="สกรีนช็อต 2026-09-16 230346" src="https://github.com/user-attachments/assets/f83b6251-ca16-4874-9c38-dcc9ca7c595f" />

---

## 5. คำถามท้ายการทดลองเพื่อการประเมินผล
1. เหตุใดการคำนวณสเกลเซนเซอร์จึงควรทำที่ฝั่ง Kestrel Server แทนที่จะคำนวณบนไมโครคอนโทรลเลอร์ ESP32 ตั้งแต่แรก?

   **ตอบ** เพื่อลดภาระการประมวลผลของบอร์ด ESP32 และทำให้เราสามารถปรับตั้งค่า (Calibrate) ใหม่ผ่าน Web API ได้ตลอดเวลา โดยไม่ต้องเสียเวลาแฟลชโค้ดลงบอร์ดใหม่
   
2. จากการทำ HTTP Forensics หากไม่มีการตรวจสอบเงื่อนไข `RawMax <= RawMin` ในโค้ด จะเกิด Exception ชนิดใดขึ้นในภาษา C# และส่งผลต่อการทำงานของเซิร์ฟเวอร์อย่างไร?

   **ตอบ** จะเกิดข้อผิดพลาด "การหารด้วยศูนย์" (Divide by Zero) เพราะในสูตรคำนวณ ตัวหาร (RawMax - RawMin) จะกลายเป็น 0 ซึ่งจะส่งผลให้เซิร์ฟเวอร์ Kestrel ล่ม (Crash) และหยุดทำงานทันที
   
3. อธิบายสาเหตุทางเทคนิคว่าทำไมคำขอ HTTP POST ที่ไม่มี Header `Content-Type: application/json` จึงถูกปฏิเสธด้วยรหัสสถานะ `415 Unsupported Media Type`?

   **ตอบ** เพราะเซิร์ฟเวอร์ไม่รู้ว่าข้อมูลที่ส่งมาเป็นไฟล์ประเภทไหน (อ่านไม่ออก) การระบุ application/json จึงเป็นการบอกให้เซิร์ฟเวอร์เตรียมตัวแปลงรหัสข้อมูลให้ถูกต้อง หากไม่บอก เซิร์ฟเวอร์จะปฏิเสธคำขอเพื่อความปลอดภัย


