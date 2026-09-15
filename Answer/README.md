# Answer — Week 09: SPI OLED & Kestrel UI

รายงานผลการทดลองและคำตอบคำถามท้ายบท
**รหัสนักศึกษา 67030098** · branch `67030098-HW`

---

## สารบัญคำตอบ

| ใบงาน | หัวข้อ | ไฟล์คำตอบ | โค้ดที่เกี่ยวข้อง | สถานะ |
| :---: | :--- | :--- | :--- | :---: |
| **9.1** | SPI OLED Deconstructed Bring-up & Framebuffer Forensics | **[answer-lab-9-1.md](answer-lab-9-1.md)** | [`Code/Lab9-1_OLED_BringUp/`](../Code/Lab9-1_OLED_BringUp/) | ✅ เสร็จ |
| **9.2** | Kestrel Calibration Engine & Display API | **[answer-lab-9-2.md](answer-lab-9-2.md)** | [`Code/Lab9-2_Kestrel_Webserver/`](../Code/Lab9-2_Kestrel_Webserver/) | ✅ เสร็จ |

> ใบงาน 9.3 และ 9.4 เป็นรายละเอียดเพิ่มเติม (ไม่อยู่ในขอบเขตงานที่ส่งครั้งนี้)

---

## สรุปเนื้อหาแต่ละใบงาน

### [ใบงาน 9.1](answer-lab-9-1.md) — SPI OLED Bring-up

* การต่อวงจร 7 ขา และบทบาทของสาย DC / RES / CS
* ท่อส่งสัญญาณระดับล่าง: `oled_spi_init()` → `oled_send_cmd()` → `oled_send_data()`
* Magic Sequence และเหตุผลที่ Charge Pump (`0x8D`, `0x14`) สำคัญที่สุด
* สูตร Bitwise mapping: `index = x + (y/8)*128`, `bit = y % 8`
* **คำตอบปริศนาภาพกลับหัว 180°** — สาเหตุจากการละคำสั่ง `0xA1` และ `0xC8`
* ตาราง Hex Dump ของตัวอักษร `'H'` พร้อม Bit-to-Pixel Reconstruction
* คำตอบคำถามท้ายบท 3 ข้อ

### [ใบงาน 9.2](answer-lab-9-2.md) — Kestrel Calibration & Display API

* Two-Point Linear Calibration พร้อมกลไกกัน Divide-by-Zero และ Clamping
* REST Endpoint ครบ 3 เส้นทาง พร้อม Raw HTTP Response ที่บันทึกจากการรันจริง
* **Fault Injection ครบ 4 กรณี** (400 / 400 / 415 / 404) — เซิร์ฟเวอร์ไม่ล่ม
* บันทึกบั๊กการ escape สตริงบน PowerShell ที่เจอระหว่างทดสอบ
* คำตอบคำถามท้ายบท 3 ข้อ (รวมประเด็น `NaN` ที่ยืนยันด้วยการทดลองจริง)
