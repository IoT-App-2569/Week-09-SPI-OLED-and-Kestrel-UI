
### ผลการทดลองแลป

ข้อ 3.1 
#### จุดตรวจสอบที่ 1 (Checkpoint 3.1 Standalone / Edge Computing Mode)
ให้นักศึกษาสังเกตการทำงานบนบอร์ดจริงโดยที่ **ยังไม่ต้องเปิด Kestrel Web Server**
1. **บน Serial Monitor** ต้องเห็นข้อมูลสตรีมตัวเลขออกมาอย่างต่อเนื่อง เช่น
   ```text
   ADC:1840,4520
   ADC:1852,4570
   ADC:2048,4620
   ```
2. **บนหน้าจอ OLED ทางกายภาพ**
   - โซนที่ 1 (Header)  แสดงรหัสนักศึกษาของตนเองอย่างชัดเจน
   - โซนที่ 2 (Gauge) เมื่อหมุนลูกบิด Potentiometer ตัวเลข `RAW` และแถบสี่เหลี่ยม Gauge Bar ต้องขยับตามการหมุนอย่างลื่นไหลไม่มีกระตุก
   - โซนที่ 3 (Footer) ต้องแสดงข้อความ `EDGE: LOCAL EDGE` เนื่องจากยังไม่ได้เชื่อมต่อ Kestrel

### วิดีโอแสดงผลการทดลอง
[https://youtube.com/shorts/7fcSItvpx5M?feature=share](https://youtube.com/shorts/7fcSItvpx5M?feature=share)


ข้อ 3.2
#### จุดตรวจสอบที่ 2 (Checkpoint 3.2 Transition to Cloud Computing Mode)
1. **ใน Terminal ของ Kestrel** ต้องขึ้นข้อความว่าเปิดพอร์ต Serial สำเร็จ เช่น `เชื่อมต่อพอร์ต COMxx สำเร็จ!`
2. **สังเกตหน้าจอ OLED ทางกายภาพ**
   - โซนที่ 3 (Footer) ต้อง **เปลี่ยนข้อความจาก `EDGE: LOCAL EDGE` กลายเป็น `CLOUD: READY` ทันที!**
   - นี่คือเครื่องยืนยันว่า ESP32 ได้รับแพ็กเก็ตตอบกลับ `SET:xx:READY` จาก Kestrel และระบบได้ยกระดับเข้าสู่ **Cloud Computing Mode** สำเร็จแล้ว!
1. **ทดสอบปิด Kestrel ด้วย `Ctrl+C`**
   - ภายในเวลาประมาณ 1.5 วินาที หน้าจอ OLED จะต้องดีดกลับไปเป็น `EDGE: LOCAL EDGE` โดยอัตโนมัติ
   - เมื่อรัน `dotnet run` ใหม่อีกครั้ง จอ OLED จะต้องกลับมาเป็น `CLOUD: READY` โดยที่เฟิร์มแวร์ ESP32 ไม่ค้างหรือไม่ต้องกดปุ่มรีเซ็ตฮาร์ดแวร์เลย

  ### วิดีโอแสดงผลการทดลอง
  [https://youtube.com/shorts/mTY5RB4J5Z0](https://youtube.com/shorts/mTY5RB4J5Z0)
