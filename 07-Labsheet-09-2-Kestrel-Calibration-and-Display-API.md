# ใบงานการทดลองที่ 9.2 (Lab 9.2)
### การพัฒนาเอนจินปรับเทียบเซนเซอร์และ API ควบคุมการแสดงผลบน Kestrel Web Server พร้อมการพิสูจน์หลักฐานเครือข่าย (HTTP Payload Forensics)

>[!INFO] **คำชี้แจง** 
>ในใบงานนี้ นักศึกษาจะได้ต่อยอดเว็บเซิร์ฟเวอร์ Kestrel (.NET 8 Minimal API) จากสัปดาห์ที่ 8 
>โดยเพิ่มบริการ **Sensor Calibration Engine** เพื่อแปลงค่าแอนะล็อกดิบจาก Potentiometer ให้อยู่ในสเกลมาตรฐาน 
>และสร้าง **Display Control API** เพื่อให้ผู้ใช้งานสามารถส่งข้อความจากหน้าเว็บย้อนกลับไปสั่งการหน้าจอ OLED ได้ 
>พร้อมทั้งฝึกกระบวนการ **HTTP Packet & Payload Forensics** เพื่อตรวจสอบความถูกต้องของข้อมูลที่วิ่งบนเครือข่าย

---

## 1. วัตถุประสงค์การทดลอง (Objectives)
1. เข้าใจหลักการและสามารถเขียนเอนจินคำนวณการปรับเทียบเซนเซอร์เชิงเส้นแบบสองจุด (**Two-Point Linear Calibration**) ในภาษา C# ได้
2. สามารถสร้าง Minimal API Endpoint สำหรับการรับพารามิเตอร์ Calibrate (`POST /api/potentiometer/calibrate`) ได้
3. สามารถสร้าง Endpoint สำหรับส่งข้อความสั่งการหน้าจอ OLED (`POST /api/oled/message`) ได้
4. สามารถตรวจสอบและตรวจพิสูจน์หลักฐานแพ็กเก็ตเครือข่าย (**HTTP Request/Response Forensics**) ผ่าน `curl` หรือ Browser DevTools ได้
5. เข้าใจและป้องกันข้อผิดพลาดทางคณิตศาสตร์ (เช่น การหารด้วยศูนย์ Divide-by-Zero และ Out-of-Bounds Clamping) ในบริการ IoT

---

## 2. โครงสร้าง REST Minimal API ที่ต้องสร้าง

```
[ Kestrel Web Server (Port 5000) ]
  ├── GET  /api/telemetry                : สตรีมค่า Raw ADC, Calibrated Value, และข้อความสถานะปัจจุบัน
  ├── POST /api/potentiometer/calibrate  : ปรับเทียบค่าศูนย์และค่าเต็มสเกล (Zero & Span Calibration)
  └── POST /api/oled/message             : สั่งส่งข้อความ Broadcast ไปยังหน้าจอ OLED ทางกายภาพ
```

---

## 3. ขั้นตอนการทดลอง (Step-by-Step Activities)

### กิจกรรมที่ 2.0: สร้างโปรเจก Kestrel web server

1. เปิด vscode หรือ antigravity แล้วทำการสร้าง terminal ขึ้นมาใหม่ จากนั้นตรวจสอบ current directory ของ terminal ปัจจุบัน

| Terminal   | Command        |
| ---------- | -------------- |
| powershell | `get-location` |
| bash       | `pwd`          |

2. ย้ายไปยังที่ที่จะสร้าง kestrel project

3. สร้าง project 

| Terminal   | Command                                                               |
| ---------- | --------------------------------------------------------------------- |
| powershell | `dotnet new web -n ESP32.Kestrel.Webserver -f net10.0 ## หรือ net8.0` |
| bash       | `dotnet new web -n ESP32.Kestrel.Webserver -f net10.0 ## หรือ net8.0` |

4. ย้ายไปยังที่ที่สร้าง kestrel project แล้ว

สำหรับ powershell และ bash
```powershell
cd ESP32.Kestrel.Webserver
```


5. ตรวจสอบโปรเจกที่สร้าง

ใน Program.cs ต้องเห็นเนื้อหาต่อไปนี้

```csharp
var builder = WebApplication.CreateBuilder(args);
var app = builder.Build();

app.MapGet("/", () => "Hello World!");

app.Run();
```



6. build project
สำหรับ powershell และ bash
```powershell
dotnet build
```

7. run project
สำหรับ powershell และ bash
```powershell
dotnet run
```

สังเกตุ url ที่ระบบแจ้งมา และให้เปิด web browser ไปที่นั่น
ต้องเห็นข้อความ "Hello World!"

ถ้ารันได้ แสดงว่า project พร้อมไปต่อ

ถ้าไม่สามารถรันได้ ให้ตรวจสอบว่าได้ทำตามขั้นตอนที่ 1-7 อย่างถูกต้องหรือไม่



### กิจกรรมที่ 2.1: การสร้างโมเดลและบริการปรับเทียบ (Calibration Service)

1. สร้างโฟลเดอร์ Services ในโปรเจกต์ ESP32.Kestrel.Webserver
2. สร้างไฟล์ใหม่ชื่อ `CalibrationService.cs` ในโฟลเดอร์ Services
3. เพิ่มเนื้อหาต่อไปนี้ลงในไฟล์ `CalibrationService.cs`

```csharp
public class CalibrationSettings
{
    public int RawMin { get; set; } = 150;     // ค่าดิบต่ำสุด (Zero Point)
    public int RawMax { get; set; } = 3950;    // ค่าดิบสูงสุด (Span Point)
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
        // ป้องกันข้อผิดพลาดการหารด้วยศูนย์
        if (newSettings.RawMax <= newSettings.RawMin)
        {
            throw new ArgumentException("RawMax ต้องมีค่ามากกว่า RawMin เสมอ!");
        }
        _settings = newSettings;
    }

    public void SetOledMessage(string msg)
    {
        _currentOledMessage = msg.Length > 20 ? msg[..20] : msg;
    }

    public double Compute(int rawAdc)
    {
        // Clamp ค่าให้อยู่ในช่วงที่กำหนด ป้องกันสเกลทะลัก
        int clamped = Math.Clamp(rawAdc, _settings.RawMin, _settings.RawMax);
        return ((double)(clamped - _settings.RawMin) / (_settings.RawMax - _settings.RawMin)) 
               * (_settings.ScaleMax - _settings.ScaleMin) + _settings.ScaleMin;
    }
}
```

5. สร้างไฟล์ `Services/DisplayMessageRequest.cs` 
6. เพิ่มเนื้อหาต่อไปนี้ลงในไฟล์ `Services/DisplayMessageRequest.cs`

```csharp
namespace ESP32.Kestrel.Webserver.Services;

public class DisplayMessageRequest
{
    public string Message { get; set; } = string.Empty;
}
```



---

### กิจกรรมที่ 2.2: การผูก Minimal API Routes (Program.cs)

ลงทะเบียน Route บน Kestrel เพื่อรองรับการเรียกใช้งาน:

```csharp
using ESP32.Kestrel.Webserver.Services;

var builder = WebApplication.CreateBuilder(args);
builder.Services.AddSingleton<CalibrationService>();
var app = builder.Build();

// Route 1: อ่านข้อมูล Telemetry
app.MapGet("/api/telemetry", (CalibrationService cal) =>
{
    int simulatedRaw = 2048; // หรือดึงจาก Serial Stream
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

// Route 2: ปรับเทียบเซนเซอร์
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

// Route 3: สั่งข้อความขึ้นหน้าจอ OLED
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

1. ถ้าโปรเจคยังรันอยู่ใน terminal ให้หยุดรัน โดยการกด Ctrl+C
2. build project ใหม่
3. รัน project 
4. เปิด web browser ไปที่ http://localhost:5000/ หรือหมายเลขพอร์ตที่แสดงใน terminal ที่รันโปรเจค
5. กด F12 เพื่อเปิด Developer Tools
6. เลือกแท็บ Network
7. กด F5 เพื่อ Refresh หน้าเว็บ
8. สังเกต Raw HTTP Response Headers


---

## 4. ขั้นตอนการตรวจสอบเชิงนิติวิทยาศาสตร์ (HTTP Payload Forensics)

> [!TIP]
> **ทำไมวิศวกรระบบสมองกลและ IoT จึงนิยมใช้ CLI (`curl.exe`) แทน Postman?**
> 1. **Zero Overhead & Built-in:** ติดตั้งมาพร้อมกับระบบปฏิบัติการ Windows 10/11 และ Linux ทันที ไม่ต้องติดตั้งโปรแกรม GUI ขนาดใหญ่
> 2. **Deep Protocol Visibility:** พารามิเตอร์ `-i` จะกาง **Raw HTTP Header** ออกมาให้เห็นบรรทัดต่อบรรทัด ทำให้ตรวจชันสูตรสถานะของ Kestrel Web Server และ Web Protocol ได้อย่างแท้จริง
> 3. **Scriptable & Automation:** สามารถเขียนสคริปต์ PowerShell/Bash เพื่อสั่งยิงทดสอบระบบซ้ำ ๆ หรือจำลองการส่งข้อมูลจากเซนเซอร์ได้อย่างอัตโนมัติ

### cURL คืออะไร
#### 1. ที่มาและความหมายของชื่อ (Etymology)

- **cURL** ย่อมาจากคำว่า **"Client URL"** (หรือเล่นคำพ้องเสียงว่า _"see URL"_)
- มีความหมายว่า **เครื่องมือฝั่งไคลเอนต์สำหรับรับส่งข้อมูลกับ URL** ผ่านเครือข่ายอินเทอร์เน็ต

---

#### 2. นิยามอย่างเป็นทางการ (Official Designation)

ในเอกสารทางการของโครงการ (และมาตรฐาน IETF / Linux Foundation) ระบุคำนิยามไว้ว่า:

> **"cURL — A command-line tool and library for transferring data with URLs"**  
> _(เครื่องมือบรรทัดคำสั่งและไลบรารีสำหรับการรับส่งข้อมูลโดยใช้ไวยากรณ์ของ URL)_

#### 3. ตัวโปรแกรม cURL แบ่งออกเป็น 2 ส่วนหลัก
    1. **`curl` (CLI Tool):** ตัวโปรแกรมคำสั่งใน Terminal ที่เราใช้งานกัน
    2. **`libcurl` (C Engine Library)** 
       ไลบรารีภาษา C เบื้องหลัง ซึ่งเป็นหนึ่งในไลบรารีเครือข่ายที่ถูกนำไปฝัง (Embed) อยู่ในอุปกรณ์มากที่สุดในโลก เช่น ในระบบปฏิบัติการ iOS, Android, รถยนต์ Tesla, โทรทัศน์ Smart TV, เกมคอนโซล ไปจนถึงยานสำรวจดาวอังคาร (Mars Rover) ของ NASA

### โครงสร้างและไวยากรณ์ของคำสั่ง `curl.exe` บน Windows PowerShell

| พารามิเตอร์    | ความหมายและหน้าที่ทางเทคนิค                                                                                                                                          |
| :------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `curl.exe`     | เรียกโปรแกรม cURL ของระบบ (ต้องพิมพ์ `.exe` บน PowerShell เพื่อป้องกันการชนกับ Alias ภายใน)                                                                          |
| `-i` (Include) | **คำสั่งสำคัญของนักนิติวิทยาศาสตร์!** สั่งให้แสดงผล Raw HTTP Response Headers ทั้งหมด                                                                                |
| `-X <METHOD>`  | ระบุ HTTP Method เช่น `-X GET` หรือ `-X POST`                                                                                                                        |
| `-H "..."`     | กำหนด Request Header โดยเฉพาะ `-H "Content-Type: application/json"` เพื่อแจ้งเซิร์ฟเวอร์ว่า Body คือ JSON                                                            |
| `-d '...'`     | ข้อมูล Body/Payload ที่จะส่งไป **(ข้อควรระวัง: ต้องใช้ Single-Quote `'...'` ครอบก้อน JSON ทั้งหมด เพื่อไม่ให้ PowerShell ตีความเครื่องหมาย `{ }` หรือ `"` ผิดพลาด)** |

---

### กิจกรรมนิติวิทยาศาสตร์ 2.1  ทดสอบเรียกใช้งาน API ครบทั้ง 3 รูปแบบ

ให้นักศึกษาเปิด **PowerShell** หรือ **Terminal** (แยกอีกหน้าต่างหนึ่ง โดยปล่อยให้หน้าต่าง `dotnet run` ทำงานอยู่) แล้วทดสอบตามลำดับ:

#### 1. ตรวจสอบ Telemetry ปัจจุบัน (HTTP GET)

**คำสั่งด้วย `curl.exe`**
```powershell
curl.exe -i -X GET http://localhost:5117/api/telemetry
```
*(หมายเหตุ: เปลี่ยน `5117` เป็นหมายเลขพอร์ตที่หน้าจอ `dotnet run` ของตนเองระบุไว้ เช่น `5000` หรือ `5117`)*

**หรือคำสั่งด้วย PowerShell Native (`Invoke-RestMethod`)**
```powershell
Invoke-RestMethod -Uri http://localhost:5117/api/telemetry -Method Get
```

**ตัวอย่าง Raw Response ที่ต้องสังเกต**
```http
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Date: Sun, 13 Sep 2026 ...
Server: Kestrel

{"raw":2048,"calibrated":50.0,"unit":"%","displayMsg":"SYSTEM READY","timestamp":"2026-09-13T..."}
```
<img width="1855" height="1138" alt="image" src="https://github.com/user-attachments/assets/6ad72b2f-3996-434e-9567-5c5c89144531" />

---

#### 2. ทำการ Calibrate เซนเซอร์ใหม่ (HTTP POST พร้อม JSON Body)
จำลองการตั้งค่าช่วงแอนะล็อกดิบ 200 ถึง 3800 และสเกลเป็น 0 ถึง 1000 RPM

**คำสั่งด้วย `curl.exe`**
```powershell
curl.exe -i -X POST http://localhost:5117/api/potentiometer/calibrate `
  -H "Content-Type: application/json" `
  -d '{"rawMin": 200, "rawMax": 3800, "scaleMin": 0, "scaleMax": 1000, "unit": "RPM"}'
```

**หรือคำสั่งด้วย PowerShell Native (`Invoke-RestMethod`)**
```powershell
Invoke-RestMethod -Uri http://localhost:5117/api/potentiometer/calibrate -Method Post -ContentType "application/json" -Body '{"rawMin": 200, "rawMax": 3800, "scaleMin": 0, "scaleMax": 1000, "unit": "RPM"}'
```

**ตัวอย่าง Raw Response ที่ได้รับ**
```http
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Server: Kestrel

{"status":"success","settings":{"rawMin":200,"rawMax":3800,"scaleMin":0,"scaleMax":1000,"unit":"RPM"}}
```
<img width="1855" height="1138" alt="image" src="https://github.com/user-attachments/assets/ccb8320b-774f-4e95-9b40-931fa8162e6b" />

---

#### 3. ส่งข้อความใหม่ไปแสดงบนหน้าจอ OLED (HTTP POST)

**คำสั่งด้วย `curl.exe`**
```powershell
curl.exe -i -X POST http://localhost:5117/api/oled/message `
  -H "Content-Type: application/json" `
  -d '{"message":"Hello OLED"}'
```

**หรือคำสั่งด้วย PowerShell Native (`Invoke-RestMethod`)**
```powershell
Invoke-RestMethod -Uri http://localhost:5117/api/oled/message -Method Post -ContentType "application/json" -Body '{"message":"Hello OLED"}'
```

**ตัวอย่าง Raw Response ที่ได้รับ**
```http
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Server: Kestrel

{"status":"success","current":"Hello OLED"}
```
<img width="1053" height="436" alt="image" src="https://github.com/user-attachments/assets/ead5a771-c2a0-4ed1-8ff2-7ae38f2f689c" />

---

> [!WARNING]
> **คลินิกตรวจบั๊กการใช้คำสั่ง (Forensic Troubleshooting Box):**
> * **เกิดข้อผิดพลาด `415 Unsupported Media Type`:**  
>   เกิดจากลืมใส่ `-H "Content-Type: application/json"` ทำให้ Kestrel ไม่ยอมรับข้อมูล Body
> * **เกิดข้อผิดพลาด `ParserError: Unexpected token '}'`:**  
>   เกิดจากการใส่เครื่องหมาย Double-Quote ซ้อนกันใน PowerShell ให้แก้ไขโดยใช้ **Single-Quote `'{"message":"..."}'`** ครอบข้อความ JSON เสมอ!
> * **เกิดข้อผิดพลาด `404 Not Found`:**  
>   เกิดจากพิมพ์ URL ผิด เช่น พิมพ์ `/api/oled/` ตกคำว่า `message`

---

### กิจกรรมนิติวิทยาศาสตร์ 2.2: Fault Injection & Vulnerability Probe (การจงใจฉีดข้อมูลวิกฤต)

ในฐานะวิศวกร เราต้องทดสอบว่าระบบจะรับมือกับข้อมูลผิดพลาดได้อย่างปลอดภัยหรือไม่

1. **ทดสอบป้อนค่าสเกลผิดตรรกะ (Span Point น้อยกว่า Zero Point)**
   ```powershell
   curl.exe -i -X POST http://localhost:5117/api/potentiometer/calibrate `
     -H "Content-Type: application/json" `
     -d '{"rawMin": 4000, "rawMax": 1000, "scaleMin": 0, "scaleMax": 100, "unit": "%"}'
   ```
   * **ผลที่คาดหวัง:** เซิร์ฟเวอร์ต้องตอบกลับด้วย **`400 Bad Request`** พร้อมข้อความเตือน `"RawMax ต้องมีค่ามากกว่า RawMin เสมอ!"` โดยที่เซิร์ฟเวอร์ Kestrel **ไม่ล่ม (No Server Crash)**!

2. **ทดสอบส่งข้อความว่างเปล่า:**
   ```powershell
   curl.exe -i -X POST http://localhost:5117/api/oled/message `
     -H "Content-Type: application/json" `
     -d '{"message":""}'
   ```
   * **ผลที่คาดหวัง:** ได้รับ **`400 Bad Request`** แจ้งว่าข้อความต้องไม่ว่างเปล่า

---

## 5. คำถามท้ายการทดลองเพื่อการประเมินผล
1. เหตุใดการคำนวณสเกลเซนเซอร์จึงควรทำที่ฝั่ง Kestrel Server แทนที่จะคำนวณบนไมโครคอนโทรลเลอร์ ESP32 ตั้งแต่แรก?
```
การบริหารจัดการและการอัปเดตแบบรวมศูนย์ (Centralized Configuration & Management)

หากต้องมีการเปลี่ยนค่าพารามิเตอร์การปรับเทียบ (เช่น ค่า Zero/Span หรือเปลี่ยนหน่วยวัดจาก % เป็น RPM) วิศวกรสามารถอัปเดตหรือส่ง API Request มาที่ Kestrel Server ได้ทันทีโดยไม่ต้องแฟลชโค้ด (Re-flash Firmware) ใหม่ลงใน ESP32 ทุกๆ เครื่องที่ติดตั้งอยู่ตามจุดต่างๆ

การลดภาระประมวลผลของไมโครคอนโทรลเลอร์ (Resource & Power Optimization)

ไมโครคอนโทรลเลอร์อย่าง ESP32 มีทรัพยากร (CPU/RAM) และพลังงานที่จำกัด การให้ ESP32 ทำหน้าที่เพียงอ่านค่าแอนะล็อกดิบ (Raw ADC) แล้วส่งต่อไปยังเซิร์ฟเวอร์ จะช่วยลดภาระงานคำนวณเลขทศนิยม (Floating-point Operation) และประหยัดพลังงานได้มากกว่า

ความยืดหยุ่นในการขยายระบบ (Scalability & Data Integrity)

ค่า Raw Data ที่ส่งมาจาก ESP32 คือข้อมูลบริสุทธิ์ (Raw Ground Truth) ซึ่งสามารถนำมาเข้าเอนจินการปรับเทียบย้อนหลัง (Post-processing) หรือประยุกต์ใช้โมเดลการปรับเทียบขั้นสูง (เช่น Non-linear / Polynomial Calibration) บน Server ได้หลากหลายรูปแบบ โดยไม่เสียข้อมูลดั้งเดิมไป
```
2. จากการทำ HTTP Forensics หากไม่มีการตรวจสอบเงื่อนไข `RawMax <= RawMin` ในโค้ด จะเกิด Exception ชนิดใดขึ้นในภาษา C# และส่งผลต่อการทำงานของเซิร์ฟเวอร์อย่างไร?
```
ชนิดของ Exception

หากไม่มีการตรวจจับ RawMax <= RawMin แล้วมีการส่งค่า RawMax เท่ากับ RawMin เข้ามา (เช่น RawMax = 100, RawMin = 100) ในขั้นตอนการคำนวณ Compute() ตัวหารจะเป็น (_settings.RawMax - _settings.RawMin) ซึ่งเท่ากับ 0

ใน C# การหารตัวเลขประเภท Floating-point (double) ด้วย 0.0 จะไม่โยน (Throw) DivideByZeroException แต่จะคืนค่าเป็น double.PositiveInfinity หรือ double.NaN (Not a Number)

แต่หากค่าถูกนำไป Cast เป็น int ในบางจุด หรือหากมีการตรวจสอบขอบเขตขัดแย้งกัน อาจทำให้เกิด DivideByZeroException (กรณีใช้ integer arithmetic) หรือเกิด ArgumentException จากฟังก์ชัน Math.Clamp() (เนื่องจาก Math.Clamp(value, min, max) จะโยน Exception ทันทีถ้า min > max)

ผลกระทบต่อการทำงานของเซิร์ฟเวอร์

กรณีค่ากลายเป็น NaN หรือ Infinity ค่าผลลัพธ์ Telemetry ที่ส่งออกไปในรูปแบบ JSON จะผิดเพี้ยน ทำให้แอปพลิเคชันฝั่ง Client หรือ Dashboard ที่รับข้อมูลไปใช้งานทำงานผิดพลาด (Data Corruption)

กรณีเกิด Unhandled Exception หากไม่มีการ try-catch จัดการ Exception ดังกล่าว Kestrel Server จะตอบกลับไคลเอนต์ด้วย HTTP 500 Internal Server Error ซึ่งแสดงถึงความบกพร่องของระบบ และหากเกิดขึ้นซ้ำๆ หรือไม่มี Middleware รองรับ อาจส่งผลกระทบต่อความเสถียร (Availability) ของบริการ IoT ได้
```
3. อธิบายสาเหตุทางเทคนิคว่าทำไมคำขอ HTTP POST ที่ไม่มี Header `Content-Type: application/json` จึงถูกปฏิเสธด้วยรหัสสถานะ `415 Unsupported Media Type`?
```
การตรวจสอบ Content Negotiation บน Kestrel Framework

ใน Minimal API ของ .NET (Kestrel) เมื่อกำหนดรับ Parameter เป็น C# Object (เช่น DisplayMessageRequest req หรือ CalibrationSettings newSettings) ตัว API Framework จะใช้ Body Binding Engine (System.Text.Json) ในการแปลง (Deserialize) ข้อมูลจาก HTTP Request Body ให้เป็น C# Object

หน้าที่ของ Header Content-Type

Header Content-Type application/json เป็นตัวระบุตกลงสัญญา (Contract) ในชั้น Application Layer บอกให้ Kestrel ทราบว่าข้อมูลสตรีมไบต์ (Raw Stream) ที่ส่งมาใน Request Body อยู่ในรูปแบบข้อความโครงสร้าง JSON

สาเหตุที่เกิด HTTP 415

หากผู้ใช้ละเลยการส่ง Header Content-Type ตัว Kestrel จะไม่สามารถยืนยัน ฟอร์แมตของข้อมูลที่ส่งมาได้ (หรือตีความว่าเป็น text/plain, application/x-www-form-urlencoded ฯลฯ) Framework จึงปฏิเสธคำขอนั้นทันทีที่ชั้น Routing/Model Binding โดยส่งรหัสสถานะ 415 Unsupported Media Type เพื่อป้องกันไม่ให้เซิร์ฟเวอร์เสียเวลาอ่านหรือพยายาม Deserialize ข้อมูลที่ไม่ถูกต้องตามข้อตกลง
```

