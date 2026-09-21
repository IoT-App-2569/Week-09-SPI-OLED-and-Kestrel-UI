# สัปดาห์ที่ 09: สถาปัตยกรรมหน้าจอ SPI OLED, 1KB Framebuffer และ Edge Kestrel UI

## 📂 โครงสร้างและจุดนำทางของ Repository (Repository Navigation)



* **[`/Lab/Lab9-1_OLED_BringUp/`](https://github.com/pornprompoh/Week-09-SPI-OLED-and-Kestrel-UI/tree/67030323/Lab/ESP32.Kestrel.Webserver)**: โฟลเดอร์โปรเจกต์ ESP-IDF (ภาษา C) สำหรับควบคุมการแสดงผลหน้าจอ OLED ผ่านบัส SPI


* **[`/Lab/ESP32.Kestrel.Webserver/`](https://github.com/pornprompoh/Week-09-SPI-OLED-and-Kestrel-UI/tree/67030323/Lab/Lab9-1_OLED_BringUp)**: โฟลเดอร์โปรเจกต์ .NET (ภาษา C#) สำหรับการสร้างเว็บเซิร์ฟเวอร์ด้วย Kestrel Minimal API

* **[`/Lab/Lab9-3-ESP32-ClosedLoop/`](https://github.com/pornprompoh/Week-09-SPI-OLED-and-Kestrel-UI/tree/67030323/Lab/Lab9-3-ESP32-ClosedLoop)**: โฟลเดอร์โปรเจกต์ ESP-IDF (ภาษา C) สำหรับระบบวงปิด (Closed-Loop) ที่ทำงานร่วมกับเซิร์ฟเวอร์แบบ Real-time พร้อมกลไก Hybrid Edge-Cloud Fallback

* **[`/Report/`](https://github.com/pornprompoh/Week-09-SPI-OLED-and-Kestrel-UI/tree/67030323/Report)**: โฟลเดอร์จัดเก็บเอกสารรายงานการทดลองและหลักฐานการทำงาน


* [`Lab1.md`](https://github.com/pornprompoh/Week-09-SPI-OLED-and-Kestrel-UI/blob/67030323/Report/Lab1.md): เอกสารอธิบายการทดลองและผลลัพธ์ของการเปิดใช้งาน (Bring-Up) หน้าจอ OLED ผ่าน SPI


* [`Lab2.md`](https://github.com/pornprompoh/Week-09-SPI-OLED-and-Kestrel-UI/blob/67030323/Report/Lab2.md): เอกสารรายงาน, การวิเคราะห์นิติวิทยาศาสตร์เครือข่าย (Forensics) และการทดสอบ Payload สำหรับ Kestrel Web Server

* **[`Lab3.md`](https://github.com/pornprompoh/Week-09-SPI-OLED-and-Kestrel-UI/blob/67030323/Report/Lab3.md): เอกสารรายงานการบูรณาการระบบวงปิด, การตรวจสอบความสอดคล้องของข้อมูล (Co-Verification) และการตรวจวัดความหน่วงเวลา (Latency Forensics)**

* [`/img/`](https://github.com/pornprompoh/Week-09-SPI-OLED-and-Kestrel-UI/tree/67030323/Report/img): รูปภาพ Screenshot ของผลลัพธ์จาก Serial Monitor, HTTP Status Codes และการทดสอบผ่าน Terminal

---

## 🚀 วิธีการรันโปรเจกต์ (How to Run)

### การรัน Kestrel Web Server (.NET)

1. ไปที่โฟลเดอร์ของเซิร์ฟเวอร์:
```bash
cd Lab/ESP32.Kestrel.Webserver

```


2. Build และ Run โปรเจกต์:
```bash
dotnet build
dotnet run

```


3. เซิร์ฟเวอร์จะเริ่มทำงานที่ `http://localhost:5000` (หรือพอร์ตที่ถูกกำหนดไว้) สามารถทดสอบ API ผ่าน PowerShell ได้ดังนี้:
```powershell
Invoke-RestMethod -Uri http://localhost:5000/api/telemetry -Method Get

```



### การแฟลชบอร์ด ESP32 (ESP-IDF)

1. เปิดหน้าต่าง IDF PowerShell Environment
2. ไปที่โฟลเดอร์ของโปรเจกต์ฮาร์ดแวร์:
```bash
cd Lab/Lab9-1_OLED_BringUp

```


3. Build และ Flash โค้ดลงไมโครคอนโทรลเลอร์:
```bash
idf.py build
idf.py flash monitor

```