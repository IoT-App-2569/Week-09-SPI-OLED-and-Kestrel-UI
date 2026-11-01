# Lab 9.3: Closed-Loop IoT Integration & Telemetry Co-Verification
**ผู้จัดทำ:** นายกฤษฏิณัช สำราญกิจ (67030270) & นายอัฐวุฒิ บัวเรียน (67030359)

ขั้นตอนการรันระบบวงปิด:
1. แฟลชบอร์ด ESP32
2. รัน Kestrel Server: `dotnet run` (Port 5117)
3. รัน Serial Bridge: `python3 serial_bridge.py`
4. เปิดเบราว์เซอร์ดู SVG Gauge ที่ `http://localhost:5117`
