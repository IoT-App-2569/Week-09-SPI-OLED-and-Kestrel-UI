# Answer — Week 09: SPI OLED & Kestrel UI

รายงานผลการทดลองและคำตอบคำถามท้ายบท
รหัสนักศึกษา 67030098 · branch `67030098-HW`

---

## สารบัญ

| ใบงาน | หัวข้อ | ไฟล์คำตอบ | โค้ด | สถานะ |
| :---: | :--- | :--- | :--- | :--- |
| 9.1 | SPI OLED Bring-up | [answer-lab-9-1.md](answer-lab-9-1.md) | [Code/Lab9-1_OLED_BringUp/](../Code/Lab9-1_OLED_BringUp/) | เสร็จ ทดสอบบนบอร์ดจริงแล้ว |
| 9.2 | Kestrel Calibration & Display API | [answer-lab-9-2.md](answer-lab-9-2.md) | [Code/Lab9-2_Kestrel_Webserver/](../Code/Lab9-2_Kestrel_Webserver/) | เสร็จ |
| 9.3 | Closed-Loop Integration & Edge-Cloud Fallback | [answer-lab-9-3.md](answer-lab-9-3.md) | [Code/Lab9-3_ClosedLoop/](../Code/Lab9-3_ClosedLoop/) | คอมไพล์+แฟลชจริงสำเร็จ, Checkpoint 3.2 ผ่านแล้ว, Potentiometer ยังไม่ได้แก้ |

ใบงาน 9.4 ไม่ได้อยู่ในขอบเขตงานที่ส่งครั้งนี้

รูปหลักฐานทั้งหมดอยู่ใน [Image/](../Image/) — ภาพ Terminal ของ Lab 9.2 (8 ภาพ) และภาพฮาร์ดแวร์จริงของ Lab 9.1/9.3 (4 ภาพ) ฝังอยู่ในไฟล์คำตอบที่เกี่ยวข้องแล้ว

---

## สรุปย่อแต่ละใบงาน

**[9.1](answer-lab-9-1.md)** — ต่อวงจร 7 ขา เขียนไดรเวอร์ SPI เอง วาด Framebuffer 1KB จนพิมพ์ Hello World ได้ พร้อมดัมพ์ข้อมูลในแรมตรวจสอบ มีจุดที่น่าสนใจคือรูปจริงจากบอร์ดพบว่าจอ**ไม่กลับหัว**ทั้งที่โค้ดตั้งใจให้กลับหัวตามทฤษฎี วิเคราะห์สาเหตุไว้ในไฟล์คำตอบ

**[9.2](answer-lab-9-2.md)** — สร้าง Calibration Engine บน Kestrel พร้อม API 3 เส้นทาง ทดสอบทั้งกรณีปกติและกรณีป้อนข้อมูลผิด (fault injection) ครบ 4 แบบ เซิร์ฟเวอร์ไม่ล่ม มีภาพหน้าจอ terminal จริงประกอบทุกขั้นตอน

**[9.3](answer-lab-9-3.md)** — รวม Lab 9.1+9.2 เป็นระบบวงปิดเต็มรูปแบบ พร้อมกลไกสลับ Edge/Cloud อัตโนมัติ เจอบั๊ก 3 จุดระหว่างทำ (ชื่อไฟล์ผิด, race condition, serial port ปิดช้า) แก้ไว้หมดแล้ว ทดสอบกับบอร์ดจริงจนถึงขั้น Checkpoint 3.2 สำเร็จ มีรูปยืนยัน แต่ Potentiometer ยังอ่านค่าไม่ได้ ต้องแก้การต่อสายต่อ
