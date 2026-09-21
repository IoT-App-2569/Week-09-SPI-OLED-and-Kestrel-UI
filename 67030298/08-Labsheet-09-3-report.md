# รายงานผลการทดลอง Lab 9.3
### การรวมระบบวงปิดแบบครบวงจร การตรวจสอบความสอดคล้องของข้อมูลและเวลาหน่วง

**รหัสนักศึกษา** : 67030298

---

## ผลการทดลอง Checkpoint 3.1 — Standalone / Edge Computing Mode

ทดสอบการทำงานบนบอร์ดจริงโดย **ยังไม่เปิด Kestrel Web Server**

1. **ผลบน Serial Monitor** — ข้อมูลสตรีมตัวเลข `ADC:<raw>,<uptime>` แสดงอย่างต่อเนื่อง

```text
ADC:4095,89
ADC:4095,139
ADC:4095,189
ADC:4095,239
ADC:4095,289
ADC:4095,339
ADC:4095,389
ADC:4095,439
ADC:4095,489
ADC:4095,539
ADC:4095,589
ADC:4095,639
ADC:4095,689
ADC:4095,739
ADC:4095,789
ADC:4095,839
ADC:4095,889
ADC:4095,939
ADC:4095,989
ADC:4095,1039
ADC:4095,1089
ADC:4095,1139
ADC:4095,1189
ADC:4095,1239
ADC:4095,1289
ADC:4095,1339
ADC:4095,1389
ADC:4095,1439
ADC:4095,1489
ADC:4095,1539
ADC:4095,1589
ADC:4095,1639
ADC:4095,1689
ADC:3915,1739
ADC:3647,1789
ADC:3393,1839
ADC:3192,1889
ADC:3042,1939
ADC:2890,1989
ADC:2747,2039
ADC:2645,2089
ADC:2543,2139
ADC:2446,2189
ADC:2349,2239
ADC:2263,2289
ADC:2148,2339
ADC:2021,2389
ADC:1904,2439
ADC:1775,2489
ADC:1677,2539
ADC:1584,2589
ADC:1485,2639
ADC:1392,2689
ADC:1303,2739
ADC:1215,2789
ADC:1125,2839
ADC:1051,2889
ADC:977,2939
ADC:944,2989
ADC:900,3039
ADC:851,3089
ADC:835,3139
ADC:833,3189
ADC:840,3239
ADC:845,3289
ADC:885,3339
```

2. **ผลบนหน้าจอ OLED ทางกายภาพ**

   - Zone 1 (Header) แสดงรหัสนักศึกษา ☑ ชัดเจน / ☐ ไม่ชัดเจน
   - Zone 2 (Gauge) หมุน Potentiometer แล้ว RAW และ Gauge Bar ขยับตาม ☑ ลื่นไหล / ☐ กระตุก
   - Zone 3 (Footer) แสดงข้อความ `EDGE: LOCAL EDGE` ☑ ถูกต้อง / ☐ ไม่ถูกต้อง

   **📷 แนบรูปถ่ายหน้าจอ OLED ขณะอยู่ใน Edge Mode**

<img width="617" height="556" alt="S__72851590" src="https://github.com/user-attachments/assets/de7599c7-45d2-4f7e-8f61-1923c53195f3" />

---

## ผลการทดลอง Checkpoint 3.2 — Transition to Cloud Computing Mode

ทดสอบหลังจากรัน `dotnet run` เพื่อเปิด Kestrel Web Server

1. **Terminal ของ Kestrel** แสดงข้อความเชื่อมต่อพอร์ต Serial สำเร็จ

```text
PS C:\Users\artht\Desktop\ Week-09\Lab9_Codes\ESP32.Kestrel.Webserver> dotnet run
Building...
info: ESP32.Kestrel.Webserver.Services.SerialBridgeService[0]
      กำลังเปิดการเชื่อมต่อ Serial Port: COM3 ที่ BaudRate 115200
info: ESP32.Kestrel.Webserver.Services.SerialBridgeService[0]
      เชื่อมต่อพอร์ต COM3 สำเร็จ!
info: Microsoft.Hosting.Lifetime[14]
      Now listening on: http://localhost:5200
info: Microsoft.Hosting.Lifetime[0]
      Application started. Press Ctrl+C to shut down.
info: Microsoft.Hosting.Lifetime[0]
      Hosting environment: Development
info: Microsoft.Hosting.Lifetime[0]
      Content root path: C:\Users\artht\Desktop\ Week-09\Lab9_Codes\ESP32.Kestrel.Webserver
info: Microsoft.Hosting.Lifetime[0]
      Application is shutting down...
```

2. **หน้าจอ OLED ทางกายภาพ**

   - Zone 3 (Footer) เปลี่ยนจาก `EDGE: LOCAL EDGE` เป็น `CLOUD: READY` ☑ สำเร็จ / ☐ ไม่สำเร็จ

   **📷 แนบรูปถ่ายหน้าจอ OLED ขณะอยู่ใน Cloud Mode**

<img width="826" height="663" alt="S__72851591" src="https://github.com/user-attachments/assets/7cf75478-b795-4c9d-b6a9-3d1e1abea775" />

3. **ทดสอบปิด Kestrel (`Ctrl+C`) แล้วเปิดใหม่**

   - OLED กลับไปเป็น `EDGE: LOCAL EDGE` ภายใน ~1.5 วินาที ☑ ผ่าน / ☐ ไม่ผ่าน
   - รัน `dotnet run` ใหม่ OLED กลับมาเป็น `CLOUD: READY` โดยไม่ต้องรีเซ็ต ESP32 ☑ ผ่าน / ☐ ไม่ผ่าน

---

## ผลการทดลอง กิจกรรมที่ 3.3 — Web Dashboard

ทดสอบหน้าเว็บ Dashboard ที่ `http://localhost:5000`

1. หมุน Potentiometer แล้ว Bar Gauge บนเว็บขยับตามหน้าจอ OLED แบบ Real-time ☐ ตรงกัน / ☐ ไม่ตรง

2. พิมพ์ข้อความจากเว็บแล้ว Zone 3 บน OLED เปลี่ยนตาม ☑ สำเร็จ / ☐ ไม่สำเร็จ
   - ข้อความที่ทดสอบส่ง `Tanabordi`

   **📷 แนบรูป Screenshot ของ Web Dashboard**

**ผลการทดลอง : หมุน Potentiometer 0%**

<img width="703" height="506" alt="S__72851597" src="https://github.com/user-attachments/assets/c6756d4e-c34a-4fc7-a86e-914a9ad983d2" />

<img width="765" height="552" alt="S__72851596" src="https://github.com/user-attachments/assets/9a6c9650-65dd-4999-9494-40d4b9ace030" />

---

**ผลการทดลอง : หมุน Potentiometer 100%**

<img width="685" height="527" alt="S__72851598" src="https://github.com/user-attachments/assets/b198b099-25f1-44db-ba81-4f0bd75bce7f" />

<img width="772" height="577" alt="S__72851595" src="https://github.com/user-attachments/assets/8797f04e-eb82-4011-b981-179b5ecb549a" />

---

## ผลการทดลอง กิจกรรมที่ 4.1 — End-to-End Latency Forensics

ใช้คำสั่ง `Measure-Command` ร่วมกับ `curl.exe` เพื่อวัดความหน่วงเวลา

```powershell
Measure-Command {
    curl.exe -s -X POST http://localhost:5000/api/oled/message `
      -H "Content-Type: application/json" `
      -d '{"message":"PING TEST"}'
}
```

**ผลการวัดความหน่วง**

| รายการ | ค่าที่วัดได้ (ms) |
| :--- | :---: |
| เวลาที่ Kestrel ใช้ในการรับคำขอและอัปเดตสถานะ (`TotalMilliseconds`) | 2251.52 ms |
| เวลาที่คำสั่งถูกเขียนลงพอร์ต Serial จนจอ OLED วาดข้อความใหม่สำเร็จ | 27.6 ms |

---

## ผลการทดลอง กิจกรรมที่ 4.2 — Co-Verification Matrix

ปรับหมุน Potentiometer ไปที่ตำแหน่งต่างๆ 5 ระดับ แล้วบันทึกค่าจากระบบทั้ง 3 ส่วน

| ตำแหน่งการหมุน | ค่า Raw ADC บน ESP32 ($0-4095$) | ค่าคำนวณบน Kestrel Server (%) | ค่าบนเว็บเกจ SVG (%) | แถบ Gauge บน OLED จริง (ตรง/ไม่ตรง) | โหมดที่แสดงบน Zone 3 |
| :---: | :---: | :---: | :---: | :---: | :---: |
| หมุนซ้ายสุด ($0^\circ$) | 120 | 0.0% | 0.0% | ☑ ตรง ☐ ไม่ตรง | CLOUD: READY |
| หมุนประมาณ $45^\circ$ | 1085 | 24.6% | 24.6% | ☑ ตรง ☐ ไม่ตรง | CLOUD: READY |
| กึ่งกลาง ($90^\circ$) | 2050 | 50.0% | 50.0% | ☑ ตรง ☐ ไม่ตรง | CLOUD: READY |
| หมุนประมาณ $135^\circ$ | 3010 | 75.3% | 75.3% | ☑ ตรง ☐ ไม่ตรง | CLOUD: READY |
| หมุนขวาสุด ($180^\circ$) | 4095 | 100.0% | 100.0% | ☑ ตรง ☐ ไม่ตรง | CLOUD: READY |

---

## คำถามท้ายการทดลอง (Deep Assessment Questions)

### คำถามข้อที่ 1 — การวิเคราะห์จุดคอขวด (Bottleneck Analysis)

หากพบว่าความหน่วงเวลาโดยรวม ($\Delta T$) สูงเกิน 200 ms ความล่าช้านั้นน่าจะเกิดจากส่วนประกอบใดมากที่สุด ระหว่าง บัสฮาร์ดแวร์ SPI2 (10 MHz), พอร์ต Serial UART (115200 bps), หรือวงจรแปลงสัญญาณ ADC1 (One-Shot Mode)?
> น่าจะเกิดจาก **UART (115200 bps)** มากที่สุด เพราะมีอัตราส่งข้อมูลต่ำที่สุดในระบบ

จงอธิบายเหตุผลประกอบการคำนวณอัตราเร็วการส่งข้อมูล
> - SPI2 ส่ง 1024 bytes ใช้เวลา 1024 × 8 / 10,000,000 ≈ **0.8 ms**
> - ADC1 แปลงสัญญาณใช้เวลาแค่ประมาณ **10-20 µs**
> - UART ส่งได้ประมาณ 11,520 bytes/s (1 byte = 10 bits รวม start/stop) แพ็กเก็ตยาว ~18 bytes ใช้เวลา ~1.6 ms แต่ต้องรอจนกว่า ESP32 จะส่ง `\n` ครบบรรทัด (รอบละ 50 ms) แล้ว Kestrel ถึงจะประมวลผลและตอบกลับได้ จึงเป็นจุดคอขวดหลัก

---

### คำถามข้อที่ 2 — ประโยชน์ของสถาปัตยกรรม Hybrid Edge-Cloud Fallback

เหตุใดในระบบควบคุม IoT ทางอุตสาหกรรม (เช่น แขนกลอุตสาหกรรม หรือระบบระบายความร้อน) จึงต้องมีกลไกสลับมาประมวลผลที่ระดับ Edge ทันทีเมื่อสัญญาณขาดหายไปเกินกำหนดเวลา (Timeout)?  
> เพราะระบบอุตสาหกรรมหยุดทำงานไม่ได้แม้แค่ไม่กี่วินาที เช่น แขนกลอาจชนคนหรือชิ้นงาน ระบบระบายความร้อนหยุดอาจทำให้อุปกรณ์เสียหาย จึงต้องให้ตัว Edge ประมวลผลเองได้ทันทีโดยไม่ต้องรอ Cloud

หากไม่มีกลไกนี้จะส่งผลเสียอย่างไร?

> ESP32 จะไม่ได้รับคำสั่งกลับมา ค่าบนจอ OLED จะค้างที่ค่าเดิม ไม่อัปเดตตามสภาวะจริง เท่ากับระบบควบคุมหยุดตอบสนอง อาจนำไปสู่อุบัติเหตุหรือความเสียหายต่อเครื่องจักรได้

---

### คำถามข้อที่ 3 — ความถูกต้องของการเลือกใช้งาน ADC Channel

เหตุใดการออกแบบระบบ IoT ที่รองรับการเชื่อมต่อเครือข่ายไร้สาย (Wi-Fi Stack) จึงถูกห้ามไม่ให้ใช้ขาในกลุ่ม ADC2 โดยเด็ดขาด?
> เพราะเมื่อเปิด Wi-Fi แล้ว ฮาร์ดแวร์ภายในจะจองวงจร ADC2 ไว้ใช้ประมวลผลสัญญาณ RF แบบผูกขาด โปรแกรมผู้ใช้จึงเข้าถึง ADC2 ไม่ได้อย่างถูกต้อง

อธิบายกลไกภายในของชิป ESP32 ที่เกี่ยวข้องกับปัญหานี้
> ESP32 มี SAR ADC 2 ชุด โดย SAR ADC2 มีตัวจัดลำดับการเข้าถึงที่แชร์กับ Wi-Fi Controller พอ Wi-Fi เริ่มทำงาน ตัว Controller จะเข้าไปควบคุมวงจร ADC2 เพื่อวัดระดับสัญญาณ RF ถ้าโปรแกรมพยายามอ่านค่า ADC2 ตอนนั้นจะเกิดการแย่งทรัพยากร ค่าที่อ่านได้จะเพี้ยนหรือค้าง ส่วน ADC1 มีวงจรแยกอิสระจึงไม่มีปัญหานี้

