// ==========================================================================
//  Lab 9.2 : Kestrel Calibration Engine + Display Control API
//  รัน:  dotnet run   ->  http://localhost:5117 (ดูพอร์ตจริงใน terminal)
// ==========================================================================

using ESP32.Kestrel.Webserver.Services;

var builder = WebApplication.CreateBuilder(args);
builder.Services.AddSingleton<CalibrationService>();

var app = builder.Build();

app.MapGet("/", () => "Hello World!");

// -------------------------------------------------------------------------
// Route 1: อ่านข้อมูล Telemetry (GET /api/telemetry)
// -------------------------------------------------------------------------
app.MapGet("/api/telemetry", (CalibrationService cal) =>
{
    int simulatedRaw = 2048;                 // ใบงาน 9.3 จะเปลี่ยนเป็นค่าจริงจาก Serial
    double calibrated = cal.Compute(simulatedRaw);

    return Results.Ok(new
    {
        raw        = simulatedRaw,
        calibrated = Math.Round(calibrated, 1),
        unit       = cal.Settings.Unit,
        displayMsg = cal.CurrentOledMessage,
        timestamp  = DateTime.UtcNow
    });
});

// -------------------------------------------------------------------------
// Route 2: ปรับเทียบเซนเซอร์ (POST /api/potentiometer/calibrate)
// -------------------------------------------------------------------------
app.MapPost("/api/potentiometer/calibrate",
    (CalibrationSettings newSettings, CalibrationService cal) =>
{
    try
    {
        cal.UpdateSettings(newSettings);
        cal.SetOledMessage("CALIBRATED OK");
        return Results.Ok(new { status = "success", settings = cal.Settings });
    }
    catch (ArgumentException ex)
    {
        // ตอบ 400 โดยที่เซิร์ฟเวอร์ต้องไม่ล่ม (No Server Crash)
        return Results.BadRequest(new { status = "error", message = ex.Message });
    }
});

// -------------------------------------------------------------------------
// Route 3: สั่งข้อความขึ้นหน้าจอ OLED (POST /api/oled/message)
// -------------------------------------------------------------------------
app.MapPost("/api/oled/message", (DisplayMessageRequest req, CalibrationService cal) =>
{
    if (req is null || string.IsNullOrWhiteSpace(req.Message))
    {
        return Results.BadRequest(new { status = "error", message = "ข้อความต้องไม่ว่างเปล่า" });
    }

    cal.SetOledMessage(req.Message);
    return Results.Ok(new { status = "success", current = cal.CurrentOledMessage });
});

app.Run();
