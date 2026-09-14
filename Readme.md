# สัปดาห์ที่ 09: สถาปัตยกรรมหน้าจอ SPI OLED, 1KB Framebuffer และ Edge Kestrel UI

## 📂 โครงสร้างและจุดนำทางของ Repository (Repository Navigation)



* **[`/Lab/Lab9-1_OLED_BringUp/`](https://www.google.com/search?q=./Lab/Lab9-1_OLED_BringUp/)**: โฟลเดอร์โปรเจกต์ ESP-IDF (ภาษา C) สำหรับควบคุมการแสดงผลหน้าจอ OLED ผ่านบัส SPI


* **[`/Lab/ESP32.Kestrel.Webserver/`](https://www.google.com/search?q=./Lab/ESP32.Kestrel.Webserver/)**: โฟลเดอร์โปรเจกต์ .NET (ภาษา C#) สำหรับการสร้างเว็บเซิร์ฟเวอร์ด้วย Kestrel Minimal API


* **[`/Report/`](https://www.google.com/search?q=./Report/)**: โฟลเดอร์จัดเก็บเอกสารรายงานการทดลองและหลักฐานการทำงาน


* [`Lab1.md`](https://www.google.com/search?q=./Report/Lab1.md): เอกสารอธิบายการทดลองและผลลัพธ์ของการเปิดใช้งาน (Bring-Up) หน้าจอ OLED ผ่าน SPI


* [`Lab2.md`](https://www.google.com/search?q=./Report/Lab2.md): เอกสารรายงาน, การวิเคราะห์นิติวิทยาศาสตร์เครือข่าย (Forensics) และการทดสอบ Payload สำหรับ Kestrel Web Server


* [`/img/`](https://www.google.com/search?q=./Report/img/): รูปภาพ Screenshot ของผลลัพธ์จาก Serial Monitor, HTTP Status Codes และการทดสอบผ่าน Terminal

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