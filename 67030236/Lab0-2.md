### ภาพผลการทดลอง Raw HTTP Response Headers
<img width="1222" height="527" alt="image" src="https://github.com/user-attachments/assets/c7a9c398-7b79-4fc8-8b17-d0bf7a09fcba" />

<img width="1296" height="705" alt="image" src="https://github.com/user-attachments/assets/b11bbcbf-3926-4142-8b8d-3a4fb2bb65f9" />

<img width="1297" height="518" alt="image" src="https://github.com/user-attachments/assets/be17b798-a819-4ccb-b5e6-1d6ac9d5a2be" />


## 5. คำถามท้ายการทดลองเพื่อการประเมินผล
1. เหตุใดการคำนวณสเกลเซนเซอร์จึงควรทำที่ฝั่ง Kestrel Server แทนที่จะคำนวณบนไมโครคอนโทรลเลอร์ ESP32 ตั้งแต่แรก?
2. จากการทำ HTTP Forensics หากไม่มีการตรวจสอบเงื่อนไข `RawMax <= RawMin` ในโค้ด จะเกิด Exception ชนิดใดขึ้นในภาษา C# และส่งผลต่อการทำงานของเซิร์ฟเวอร์อย่างไร?
3. อธิบายสาเหตุทางเทคนิคว่าทำไมคำขอ HTTP POST ที่ไม่มี Header `Content-Type: application/json` จึงถูกปฏิเสธด้วยรหัสสถานะ `415 Unsupported Media Type`?
