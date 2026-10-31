
# รายงานการทดลองที่ 9.2 (Lab 9.2)

## ขั้นตอนการตรวจสอบเชิงนิติวิทยาศาสตร์ (HTTP Payload Forensics) และผลการประเมิน

---

## 1. บันทึกผลกิจกรรมนิติวิทยาศาสตร์ 2.1: ทดสอบเรียกใช้งาน API ครบทั้ง 3 รูปแบบ

### 1.1 ตรวจสอบ Telemetry ปัจจุบัน (HTTP GET)

**คำสั่ง cURL**

```powershell
curl.exe -i -X GET http://localhost:5117/api/telemetry
```

**ผลลัพธ์ Raw Response จาก Kestrel Server**

```http
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Date: Thu, 29 Oct 2026 16:00:00 GMT
Server: Kestrel

{"raw":2048,"calibrated":50.0,"unit":"%","displayMsg":"ID: 67030067","timestamp":"2026-10-29T16:00:00Z"}
```

**สรุปผล:** เซิร์ฟเวอร์ตอบกลับสถานะ `200 OK` และส่งข้อมูล Telemetry ในรูปแบบ JSON ได้สำเร็จ

### 1.2 ทำการ Calibrate เซนเซอร์ใหม่ (HTTP POST พร้อม JSON Body)

**คำสั่ง cURL**

```powershell
curl.exe -i -X POST http://localhost:5117/api/potentiometer/calibrate `
  -H "Content-Type: application/json" `
  -d '{"rawMin": 200, "rawMax": 3800, "scaleMin": 0, "scaleMax": 1000, "unit": "RPM"}'
```

**ผลลัพธ์ Raw Response จาก Kestrel Server**

```http
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Server: Kestrel

{"status":"success","settings":{"rawMin":200,"rawMax":3800,"scaleMin":0,"scaleMax":1000,"unit":"RPM"}}
```

**สรุปผล:** เซิร์ฟเวอร์รับค่าการตั้งค่า Calibration ใหม่ได้สำเร็จ โดยกำหนดช่วง Raw Sensor ตั้งแต่ 200–3800 และช่วงค่าที่แปลงแล้วตั้งแต่ 0–1000 RPM

### 1.3 ส่งข้อความใหม่ไปแสดงบนหน้าจอ OLED (HTTP POST)

**คำสั่ง cURL**

```powershell
curl.exe -i -X POST http://localhost:5117/api/oled/message `
  -H "Content-Type: application/json" `
  -d '{"message":"ID: 67030067"}'
```

**ผลลัพธ์ Raw Response จาก Kestrel Server**

```http
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Server: Kestrel

{"status":"success","current":"ID: 67030067"}
```

**สรุปผล:** เซิร์ฟเวอร์รับข้อความและตอบกลับสถานะสำเร็จ พร้อมแสดงข้อความปัจจุบันเป็น `ID: 67030067`

---

## 2. บันทึกผลกิจกรรมนิติวิทยาศาสตร์ 2.2: Fault Injection & Vulnerability Probe

### 2.1 ทดสอบป้อนค่าสเกลผิดตรรกะ (rawMin > rawMax)

**คำสั่ง cURL**

```powershell
curl.exe -i -X POST http://localhost:5117/api/potentiometer/calibrate `
  -H "Content-Type: application/json" `
  -d '{"rawMin": 4000, "rawMax": 1000, "scaleMin": 0, "scaleMax": 100, "unit": "%"}'
```

**ผลลัพธ์ Raw Response จาก Kestrel Server**

```http
HTTP/1.1 400 Bad Request
Content-Type: application/json; charset=utf-8
Server: Kestrel

{"error":"Bad Request","message":"RawMax ต้องมีค่ามากกว่า RawMin เสมอ!"}
```

**สรุปผล:** เซิร์ฟเวอร์ตรวจพบว่าค่า RawMax น้อยกว่า RawMin จึงปฏิเสธคำขอและส่งสถานะ `400 Bad Request` กลับมา

### 2.2 ทดสอบส่งข้อความว่างเปล่า ("")

**คำสั่ง cURL**

```powershell
curl.exe -i -X POST http://localhost:5117/api/oled/message `
  -H "Content-Type: application/json" `
  -d '{"message":""}'
```

**ผลลัพธ์ Raw Response จาก Kestrel Server**

```http
HTTP/1.1 400 Bad Request
Content-Type: application/json; charset=utf-8
Server: Kestrel

{"error":"Bad Request","message":"ข้อความต้องไม่เป็นค่าว่างเปล่า!"}
```

**สรุปผล:** เซิร์ฟเวอร์ตรวจพบว่าข้อความที่ส่งมาว่างเปล่า จึงปฏิเสธคำขอด้วยสถานะ `400 Bad Request`

### 2.3 ตารางสรุปผลการทดสอบ API

| กิจกรรม | HTTP Method | ผลลัพธ์ | สถานะ |
|---|---|---|---|
| ตรวจสอบ Telemetry | GET | ส่งข้อมูล JSON สำเร็จ | 200 OK |
| Calibrate เซนเซอร์ | POST | บันทึกการตั้งค่าสำเร็จ | 200 OK |
| ส่งข้อความ OLED | POST | รับข้อความสำเร็จ | 200 OK |
| ป้อนค่า RawMin มากกว่า RawMax | POST | ปฏิเสธค่าที่ไม่ถูกต้อง | 400 Bad Request |
| ส่งข้อความว่างเปล่า | POST | ปฏิเสธข้อความว่าง | 400 Bad Request |

---

## 3. คำตอบคำถามท้ายการทดลองเพื่อการประเมินผล (Review Questions)

### 1. เหตุใดการคำนวณสเกลเซนเซอร์จึงควรทำที่ฝั่ง Kestrel Server แทนที่จะคำนวณบนไมโครคอนโทรลเลอร์ ESP32 ตั้งแต่แรก?

**คำตอบ**

1. **ลดภาระของ Edge Device (ESP32):** ESP32 มีทรัพยากรประมวลผลและหน่วยความจำจำกัด การให้ ESP32 ทำหน้าที่เพียงอ่านและส่งค่าสัญญาณดิบ (Raw Sensor Value) จะช่วยประหยัดเวลาและพลังงานของตัวอุปกรณ์
2. **ความยืดหยุ่นและการบริหารจัดการศูนย์กลาง (Centralized Calibration):** หากต้องการปรับเปลี่ยนสมการสเกล ค่า Min/Max หรือหน่วยการวัด สามารถดำเนินการผ่าน API Server ได้ทันทีโดยไม่ต้อง Re-flash เฟิร์มแวร์ใหม่ลงบอร์ด ESP32
3. **ความถูกต้องและมาตรฐานเดียวกัน:** การคำนวณที่จุดเดียวช่วยป้องกันปัญหาความคลาดเคลื่อนของข้อมูล เมื่อมี Edge Device หลายตัวส่งข้อมูลเข้ามายังระบบ

### 2. จากการทำ HTTP Forensics หากไม่มีการตรวจสอบเงื่อนไข RawMax <= RawMin ในโค้ด จะเกิด Exception ชนิดใดขึ้นในภาษา C# และส่งผลต่อการทำงานของเซิร์ฟเวอร์อย่างไร?

**คำตอบ**

**Exception ที่อาจเกิดขึ้น:** หาก RawMax == RawMin จะเกิด `DivideByZeroException` เนื่องจากสูตรสเกลมีตัวหารเป็น (RawMax - RawMin) หรือเกิด `ArgumentException` / Logic Error ที่ทำให้คำนวณค่า Calibrated ติดลบหรือผิดเพี้ยน

**ผลกระทบต่อเซิร์ฟเวอร์:** หากไม่มีการทำ Validation หรือ try-catch ดักจับ Unhandled Exception จะทำให้ Kestrel คืนค่า `500 Internal Server Error` ให้กับ Client และอาจส่งผลกระทบต่อเสถียรภาพของกระบวนการทำงานบน Web Server

### 3. อธิบายสาเหตุทางเทคนิคว่าทำไมคำขอ HTTP POST ที่ไม่มี Header Content-Type: application/json จึงถูกปฏิเสธด้วยรหัสสถานะ 415 Unsupported Media Type?

**คำตอบ**

เพราะเฟรมเวิร์ก ASP.NET Core บน Kestrel Server ต้องอาศัย Header `Content-Type` ในการเลือกตัวแปลงข้อมูล (Formatter/Deserializer เช่น System.Text.Json) เพื่อแปลง Raw Request Body ไปเป็น C# Object

เมื่อ Client ไม่ส่ง Header `Content-Type: application/json` ตัว Web Framework จะไม่ทราบรูปแบบโครงสร้างของ Payload และปฏิเสธการประมวลผลด้วยรหัสสถานะ `415 Unsupported Media Type` เพื่อป้องกันความผิดพลาดในการแปลงข้อมูล

---

## 4. สรุปผลการทดลอง

จากการทดลองได้เรียนรู้การตรวจสอบการทำงานของ API ผ่าน HTTP GET และ HTTP POST โดยใช้คำสั่ง cURL เพื่อเรียกดูข้อมูล Telemetry ปรับเทียบค่าเซนเซอร์ และส่งข้อความไปยังหน้าจอ OLED

นอกจากนี้ยังได้ทดสอบการส่งข้อมูลที่ไม่ถูกต้อง (Fault Injection) เพื่อศึกษาการตรวจสอบความถูกต้องของข้อมูล (Validation) พบว่าเมื่อส่งค่าที่ผิดเงื่อนไขหรือข้อความว่างเปล่า เซิร์ฟเวอร์สามารถปฏิเสธคำขอและส่งสถานะ `400 Bad Request` กลับมาได้ ทำให้เข้าใจความสำคัญของการตรวจสอบข้อมูลก่อนนำไปประมวลผลในระบบ IoT Gateway
