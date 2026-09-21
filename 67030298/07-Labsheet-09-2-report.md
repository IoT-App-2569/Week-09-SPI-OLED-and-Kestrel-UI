# รายงานผลการทดลองที่ 9.2 (Lab 9.2)
### การพัฒนาเอนจินปรับเทียบเซนเซอร์และ API ควบคุมการแสดงผลบน Kestrel Web Server พร้อมการพิสูจน์หลักฐานเครือข่าย (HTTP Payload Forensics)

---


## 4. ขั้นตอนการตรวจสอบเชิงนิติวิทยาศาสตร์ (HTTP Payload Forensics)

### กิจกรรมนิติวิทยาศาสตร์ 2.1 ทดสอบเรียกใช้งาน API ครบทั้ง 3 รูปแบบ

#### 1. ตรวจสอบ Telemetry ปัจจุบัน (HTTP GET)
**ผลการทดลอง**

<img width="487" height="241" alt="9 2 2 1 1" src="https://github.com/user-attachments/assets/aa56b4f6-5087-4e9e-a8a5-1f17c56cdcd5" />

<img width="940" height="171" alt="9 2 2 1 1 (2)" src="https://github.com/user-attachments/assets/9c99f07e-6d56-4281-9afa-30a8c4505d72" />

#### 2. ทำการ Calibrate เซนเซอร์ใหม่ (HTTP POST พร้อม JSON Body)
**ผลการทดลอง**

<img width="937" height="175" alt="9 2 2 1 2" src="https://github.com/user-attachments/assets/65ecca61-7566-4c50-bd9b-d8ab5d61a30b" />

#### 3. ส่งข้อความใหม่ไปแสดงบนหน้าจอ OLED (HTTP POST)
**ผลการทดลอง**

<img width="937" height="155" alt="9 2 2 1 3" src="https://github.com/user-attachments/assets/e9a357bb-a7ed-4e31-8c39-17068dedbd20" />

---

### กิจกรรมนิติวิทยาศาสตร์ 2.2 Fault Injection & Vulnerability Probe (การจงใจฉีดข้อมูลวิกฤต)

#### 1. ทดสอบป้อนค่าสเกลผิดตรรกะ (Span Point น้อยกว่า Zero Point)
**ผลการทดลอง**

<img width="942" height="220" alt="9 2 2 2 1" src="https://github.com/user-attachments/assets/79bf7e4e-a8d4-46a7-88de-445df8d155a0" />

<img width="442" height="188" alt="9 2 2 2 1 (2)" src="https://github.com/user-attachments/assets/78e0aa8a-6421-49bc-b2e0-9c96654640f9" />

#### 2. ทดสอบส่งข้อความว่างเปล่า
**ผลการทดลอง**

<img width="935" height="212" alt="9 2 2 2 2" src="https://github.com/user-attachments/assets/6a6c9a45-c146-46f1-aa08-680496b9bca2" />

<img width="443" height="168" alt="9 2 2 2 2 (2)" src="https://github.com/user-attachments/assets/b4047f5e-c46f-4ff9-abec-92bdb22baaee" />

---

## 5. คำถามท้ายการทดลองเพื่อการประเมินผล

1. เหตุใดการคำนวณสเกลเซนเซอร์จึงควรทำที่ฝั่ง Kestrel Server แทนที่จะคำนวณบนไมโครคอนโทรลเลอร์ ESP32 ตั้งแต่แรก?
> เพราะช่วยลดภาระการประมวลผลของ ESP32 และหากต้องการเปลี่ยนสูตรหรือค่าปรับเทียบใหม่ ก็สามารถแก้อัปเดตที่เซิร์ฟเวอร์ได้ทันทีโดยไม่ต้องเสียเวลาไปอัปโหลดโค้ดลงบอร์ด ESP32 ใหม่

2. จากการทำ HTTP Forensics หากไม่มีการตรวจสอบเงื่อนไข RawMax <= RawMin ในโค้ด จะเกิด Exception ชนิดใดขึ้นในภาษา C# และส่งผลต่อการทำงานของเซิร์ฟเวอร์อย่างไร?
> หาก RawMax เท่ากับ RawMin จะทำให้ส่วนหารในสมการเป็นศูนย์และเกิด `DivideByZeroException` ซึ่งจะทำให้คำขอนั้นพัง และเซิร์ฟเวอร์จะตอบกลับมาเป็นสถานะ 500 Internal Server Error แทน

3. อธิบายสาเหตุทางเทคนิคว่าทำไมคำขอ HTTP POST ที่ไม่มี Header Content-Type application/json จึงถูกปฏิเสธด้วยรหัสสถานะ 415 Unsupported Media Type?
> *เพราะระบบหลังบ้านของ ASP.NET เข้มงวดเรื่องการแปลงรูปแบบข้อมูล หากไม่ระบุให้ชัดเจนว่าเป็น JSON ตัวระบบจะไม่กล้าเสี่ยงแปลงข้อมูลมั่วๆ จึงตัดจบและปฏิเสธคำขอตั้งแต่ด่านแรกเพื่อความปลอดภัย
