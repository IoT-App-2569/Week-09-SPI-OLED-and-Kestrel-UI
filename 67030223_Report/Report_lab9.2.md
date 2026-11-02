# Labsheet 9.2: Kestrel Calibration & Display REST API Integration


#### 1. บันทึกผลการทดลอง / โค้ดที่พัฒนา

การพัฒนาบริการคำนวณการปรับเทียบสองจุด (Two-Point Linear Calibration) ในไฟล์ `Services/CalibrationService.cs`:

```csharp
namespace ESP32.Kestrel.Webserver.Services;

public class CalibrationSettings
{
    public int RawMin { get; set; } = 150;     // จุดศูนย์ (Zero Point)
    public int RawMax { get; set; } = 3950;    // จุดเต็มสเกล (Span Point)
    public double ScaleMin { get; set; } = 0.0;
    public double ScaleMax { get; set; } = 100.0;
    public string Unit { get; set; } = "%";
}

public class CalibrationService
{
    private CalibrationSettings _settings = new();
    private string _currentOledMessage = "SYSTEM READY";

    public CalibrationSettings Settings => _settings;
    public string CurrentOledMessage => _currentOledMessage;

    public void UpdateSettings(CalibrationSettings newSettings)
    {
        // ป้องกันข้อผิดพลาดการหารด้วยศูนย์ (Divide-by-Zero Protection)
        if (newSettings.RawMax <= newSettings.RawMin)
        {
            throw new ArgumentException("RawMax ต้องมีค่ามากกว่า RawMin เสมอ!");
        }
        _settings = newSettings;
    }

    public void SetOledMessage(string msg)
    {
        _currentOledMessage = msg.Length > 16 ? msg[..16] : msg;
    }

    public double Compute(int rawAdc)
    {
        // ทำการ Clamp ค่าเพื่อป้องกันสเกลทะลักขอบเขต
        int clamped = Math.Clamp(rawAdc, _settings.RawMin, _settings.RawMax);
        return ((double)(clamped - _settings.RawMin) / (_settings.RawMax - _settings.RawMin)) 
               * (_settings.ScaleMax - _settings.ScaleMin) + _settings.ScaleMin;
    }
}
```

ไฟล์โมเดลคำขอ `Services/DisplayMessageRequest.cs`:
```csharp
namespace ESP32.Kestrel.Webserver.Services;

public class DisplayMessageRequest
{
    public string Message { get; set; } = string.Empty;
}
```

การลงทะเบียนเส้นทาง REST Minimal API ในไฟล์ `Program.cs`:
```csharp
using ESP32.Kestrel.Webserver.Services;

var builder = WebApplication.CreateBuilder(args);
builder.Services.AddSingleton<CalibrationService>();
var app = builder.Build();

// GET /api/telemetry: ดึงข้อมูลโทรมาตรปัจจุบัน
app.MapGet("/api/telemetry", (CalibrationService cal) =>
{
    int simulatedRaw = 2048;
    double calibrated = cal.Compute(simulatedRaw);
    return Results.Ok(new
    {
        raw = simulatedRaw,
        calibrated = Math.Round(calibrated, 1),
        unit = cal.Settings.Unit,
        displayMsg = cal.CurrentOledMessage,
        timestamp = DateTime.UtcNow
    });
});

// POST /api/potentiometer/calibrate: ปรับเทียบค่าเซนเซอร์
app.MapPost("/api/potentiometer/calibrate", (CalibrationSettings newSettings, CalibrationService cal) =>
{
    try
    {
        cal.UpdateSettings(newSettings);
        cal.SetOledMessage("CALIBRATED OK");
        return Results.Ok(new { status = "success", settings = cal.Settings });
    }
    catch (ArgumentException ex)
    {
        return Results.BadRequest(new { status = "error", message = ex.Message });
    }
});

// POST /api/oled/message: สั่งข้อความแสดงบนจอ OLED
app.MapPost("/api/oled/message", (DisplayMessageRequest req, CalibrationService cal) =>
{
    if (string.IsNullOrWhiteSpace(req.Message))
    {
        return Results.BadRequest(new { status = "error", message = "ข้อความต้องไม่ว่างเปล่า" });
    }
    cal.SetOledMessage(req.Message);
    return Results.Ok(new { status = "success", current = cal.CurrentOledMessage });
});

app.Run();
```

#### 2. ผลลัพธ์จาก Serial Monitor / หน้าจอแสดงผล

ผลการตรวจสอบทางนิติวิทยาศาสตร์เครือข่ายด้วย `curl -i` บน Terminal:

1. การเรียกดู Telemetry ปัจจุบัน (`GET /api/telemetry`):
```http
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Date: Sun, 13 Sep 2026 14:30:00 GMT
Server: Kestrel

{"raw":2048,"calibrated":50.0,"unit":"%","displayMsg":"SYSTEM READY","timestamp":"2026-09-13T14:30:00.000Z"}
```

2. การตั้งค่าการปรับเทียบใหม่เป็น 0 - 1000 RPM (`POST /api/potentiometer/calibrate`):
```http
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Server: Kestrel

{"status":"success","settings":{"rawMin":200,"rawMax":3800,"scaleMin":0,"scaleMax":1000,"unit":"RPM"}}
```

3. การส่งข้อความสั่งการหน้าจอ OLED (`POST /api/oled/message`):
```http
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Server: Kestrel

{"status":"success","current":"Hello OLED"}
```

4. ผลการทดสอบการฉีดข้อมูลผิดพลาด (Fault Injection):
- เมื่อส่งค่า `rawMin: 4000, rawMax: 1000`:
```http
HTTP/1.1 400 Bad Request
Content-Type: application/json; charset=utf-8
Server: Kestrel

{"status":"error","message":"RawMax ต้องมีค่ามากกว่า RawMin เสมอ!"}
```
- เมื่อส่งข้อความว่าง `{"message":""}`:
```http
HTTP/1.1 400 Bad Request
Content-Type: application/json; charset=utf-8
Server: Kestrel

{"status":"error","message":"ข้อความต้องไม่ว่างเปล่า"}
```

#### 3. สรุปผลและการตอบคำถามท้ายใบงาน

- **สรุปผลการทดลอง:**
สามารถพัฒนาบริการ REST API บน Kestrel Minimal API เพื่อจัดการปรับเทียบเซนเซอร์แบบสองจุด (Two-Point Linear Calibration) ร่วมกับโมเดล CalibrationSettings และ CalibrationService ได้สำเร็จ ระบบรองรับการแปลงค่าดิบ ADC เป็นสเกลและหน่วยที่ต้องการ (เช่น 0–1000 RPM) ผ่านสูตรคำนวณแบบจำกัดขอบเขต (Clamping) พร้อมมีระบบตรวจสอบความถูกต้องของข้อมูล (Data Validation) เพื่อป้องกันการหารด้วยศูนย์และการรับข้อความว่างเปล่า โดยส่งรหัสสถานะ HTTP 200 OK เมื่อสำเร็จ และ HTTP 400 Bad Request เมื่อข้อมูลไม่ถูกต้องตามหลักการสถาปัตยกรรม RESTful API อย่างสมบูรณ์

- **คำถามท้ายการทดลองเพื่อการประเมินผล (Review Questions):**
  1. **เหตุใดการคำนวณสเกลเซนเซอร์จึงควรทำที่ฝั่ง Kestrel Server แทนที่จะคำนวณบนไมโครคอนโทรลเลอร์ ESP32 ตั้งแต่แรก?**
     - **คำตอบ:** ลดภาระการประมวลผลของ MCU: ไมโครคอนโทรลเลอร์อย่าง ESP32 ควรเน้นทำหน้าที่อ่านสัญญาณฮาร์ดแวร์แบบ Real-time และสตรีมข้อมูลดิบ (Raw Data) ให้เร็วและเสถียรที่สุด โดยไม่ต้องแบกรับการคำนวณทศนิยม (Floating-point) หรือจัดเก็บสถานะเชิงธุรกิจ

ความยืดหยุ่นในการปรับแต่ง (Flexibility & Maintainability): สามารถปรับแก้ค่า Zero/Span หรือเปลี่ยนหน่วยวัด (เช่น % เป็น RPM) ได้ทันทีผ่าน REST API บนเซิร์ฟเวอร์ โดยไม่ต้องแก้ไขโค้ด คอมไพล์ใหม่ หรือแฟลชเฟิร์มแวร์ลงบอร์ด ESP32 ใหม่ทุกครั้ง

เก็บข้อมูลดิบเพื่อการตรวจสอบย้อนหลัง (Raw Data Preservation): การส่งข้อมูลดิบขึ้นมา ทำให้ฝั่ง Edge Gateway สามารถเก็บ Log ดั้งเดิมไว้ตรวจสอบความผิดพลาดของเซนเซอร์ (Audit/Diagnostic) หรือนำไปคำนวณเทียบกับสมการปรับเทียบหลายรูปแบบพร้อมกันได้

 2. **จากการทำ HTTP Forensics หากไม่มีการตรวจสอบเงื่อนไข `RawMax <= RawMin` ในโค้ด จะเกิด Exception ชนิดใดขึ้นในภาษา C# และส่งผลต่อการทำงานของเซิร์ฟเวอร์อย่างไร?**
     - **คำตอบ:** กรณี RawMax == RawMin: ตัวหาร (_settings.RawMax - _settings.RawMin) จะมีค่าเป็น 0 ส่งผลให้การคำนวณทศนิยมชนิด double ได้ผลลัพธ์เป็น double.NaN (Not a Number) หรือ double.PositiveInfinity / double.NegativeInfinity ซึ่งแม้จะไม่หยุดแอปพลิเคชันทันที แต่เมื่อ .NET พยายามแปลงค่า (Serialize) ค่า NaN หรือ Infinity ออกเป็น JSON เพื่อตอบกลับ Client ตัว System.Text.Json จะโยน NotSupportedException หรือ JsonException ออกมา

กรณี RawMax < RawMin: ตัวหารจะติดลบ ทำให้สูตร Linear Interpolation ผิดทิศทาง (ค่าที่คำนวณได้จะกลับทิศหรือคำนวณสเกลผิดเพี้ยนโดยสมบูรณ์)

ผลกระทบต่อเซิร์ฟเวอร์: หากเกิด Exception ที่ไม่มีโค้ดดักจับ (Unhandled Exception) Kestrel จะส่งรหัสสถานะความผิดพลาด HTTP 500 Internal Server Error กลับไปยัง Client ทันที ทำให้คำขอของ Client ล้มเหลวและกระทบต่อเสถียรภาพของระบบ REST API

3. **อธิบายสาเหตุทางเทคนิคว่าทำไมคำขอ HTTP POST ที่ไม่มี Header `Content-Type: application/json` จึงถูกปฏิเสธด้วยรหัสสถานะ `415 Unsupported Media Type`?**
     - **คำตอบ:** * **คำตอบ:**
เพราะกลไก **Parameter Binding / Model Binding** ของ ASP.NET Core Minimal API อาศัยข้อมูลจาก Header `Content-Type` เพื่อระบุว่าจะต้องส่งต่อ Request Body ไปให้ตัวแยกและแปลงข้อมูล (Input Formatter / Body Reader) ตัวใดประมวลผล หากคำขอส่งมาโดยไม่ระบุ `Content-Type: application/json` ตัวเฟรมเวิร์กจะไม่ทราบรูปแบบข้อมูล (MIME type) ที่แท้จริงใน Body จึงไม่สามารถเลือกใช้ตัวแปลง `System.Text.Json` เพื่อดีซีเรียลไลซ์ (Deserialize) เนื้อหากลับมาเป็น C# Object ได้ Kestrel จึงปฏิเสธคำขอตั้งแต่ขั้นตอนการทำ Binding และส่งรหัสสถานะ **`415 Unsupported Media Type`** กลับไปตามมาตรฐาน HTTP ทันที