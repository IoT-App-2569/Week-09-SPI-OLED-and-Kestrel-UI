# Answer — Week 09: SPI OLED & Kestrel UI

รายงานผลการทดลองและคำตอบคำถามท้ายบท
**รหัสนักศึกษา 67030098** · branch `67030098-HW`

---

## สารบัญคำตอบ

| ใบงาน | หัวข้อ | ไฟล์คำตอบ | โค้ดที่เกี่ยวข้อง | สถานะ |
| :---: | :--- | :--- | :--- | :---: |
| **9.1** | SPI OLED Deconstructed Bring-up & Framebuffer Forensics | **[answer-lab-9-1.md](answer-lab-9-1.md)** | [`Code/Lab9-1_OLED_BringUp/`](../Code/Lab9-1_OLED_BringUp/) | ✅ เสร็จ + ทดสอบบนฮาร์ดแวร์จริงแล้ว |
| **9.2** | Kestrel Calibration Engine & Display API | **[answer-lab-9-2.md](answer-lab-9-2.md)** | [`Code/Lab9-2_Kestrel_Webserver/`](../Code/Lab9-2_Kestrel_Webserver/) | ✅ เสร็จ |
| **9.3** | Closed-Loop IoT Integration & Hybrid Edge-Cloud Fallback | **[answer-lab-9-3.md](answer-lab-9-3.md)** | [`Code/Lab9-3_ClosedLoop/`](../Code/Lab9-3_ClosedLoop/) | ✅ คอมไพล์+แฟลชจริงสำเร็จ, Checkpoint 3.2 ยืนยันด้วยภาพ<br>⚠️ Potentiometer ยังอ่านค่าไม่ได้ (ปัญหาต่อสาย ยังไม่แก้) |

> ใบงาน 9.4 เป็นรายละเอียดเพิ่มเติม (ไม่อยู่ในขอบเขตงานที่ส่งครั้งนี้)

**หลักฐานภาพ:** [`Image/`](../Image/) — รวม 12 ภาพ: ภาพ Terminal ของใบงาน 9.2 (8 ภาพ) และภาพฮาร์ดแวร์จริงของใบงาน 9.1 และ 9.3 (4 ภาพ) ฝังอยู่ในไฟล์ Answer ที่เกี่ยวข้องแล้ว

---

## สรุปเนื้อหาแต่ละใบงาน

### [ใบงาน 9.1](answer-lab-9-1.md) — SPI OLED Bring-up

* การต่อวงจร 7 ขา และบทบาทของสาย DC / RES / CS
* ท่อส่งสัญญาณระดับล่าง: `oled_spi_init()` → `oled_send_cmd()` → `oled_send_data()`
* Magic Sequence และเหตุผลที่ Charge Pump (`0x8D`, `0x14`) สำคัญที่สุด
* สูตร Bitwise mapping: `index = x + (y/8)*128`, `bit = y % 8`
* **ภาพถ่ายฮาร์ดแวร์จริง** — ยืนยันว่า `HELLO WORLD` / `ID: 67030098` แสดงผลได้ แต่ **ไม่กลับหัวอย่างที่ทฤษฎีคาดไว้** พร้อมวิเคราะห์สาเหตุที่เป็นไปได้
* ตาราง Hex Dump ของตัวอักษร `'H'` พร้อม Bit-to-Pixel Reconstruction
* คำตอบคำถามท้ายบท 3 ข้อ

### [ใบงาน 9.2](answer-lab-9-2.md) — Kestrel Calibration & Display API

* Two-Point Linear Calibration พร้อมกลไกกัน Divide-by-Zero และ Clamping
* REST Endpoint ครบ 3 เส้นทาง พร้อม Raw HTTP Response ที่บันทึกจากการรันจริง
* **Fault Injection ครบ 4 กรณี** (400 / 400 / 415 / 404) — เซิร์ฟเวอร์ไม่ล่ม
* บันทึกบั๊กการ escape สตริงบน PowerShell ที่เจอระหว่างทดสอบ
* คำตอบคำถามท้ายบท 3 ข้อ (รวมประเด็น `NaN` ที่ยืนยันด้วยการทดลองจริง)

### [ใบงาน 9.3](answer-lab-9-3.md) — Closed-Loop Integration & Hybrid Edge-Cloud Fallback

* สถาปัตยกรรม Full-Duplex Serial Bridge: `ADC:<raw>,<uptime>\n` ↔ `SET:<percent>:<message>\n`
* Hybrid Edge-Cloud Fallback (Heartbeat Timeout 1,500 ms) พร้อมคำอธิบายเหตุผลเชิงวิศวกรรม
* **พบและแก้บั๊ก 3 จุด** — ชื่อไฟล์ผิดใน `CMakeLists.txt`, Race Condition บน `CalibrationService`, และ Serial Port ปิดช้าตอน Shutdown (พบระหว่างทดสอบกับบอร์ดจริง)
* **คอมไพล์และแฟลชลงบอร์ดจริงสำเร็จ** — Checkpoint 3.2 (Edge → Cloud) ยืนยันด้วยภาพถ่ายฮาร์ดแวร์ 3 รูป
* บันทึกปัญหาที่ยังไม่ได้แก้อย่างตรงไปตรงมา — Potentiometer ยังอ่านค่า RAW ไม่ได้ (ปัญหาการต่อขา Wiper)
* Checklist Co-Verification และ Checkpoint พร้อมสถานะจริง (ทำแล้ว/ยังไม่ได้ทำ)
* คำตอบคำถามท้ายบท 3 ข้อ (Bottleneck Analysis พร้อมคำนวณอัตราเร็ว SPI vs UART)
