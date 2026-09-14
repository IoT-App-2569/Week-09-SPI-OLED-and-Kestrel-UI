using ESP32.Kestrel.Webserver.Services;

var builder = WebApplication.CreateBuilder(args);
// ลงทะเบียน Service เพื่อให้ Minimal API ดึงไปใช้งานได้ (Dependency Injection)
builder.Services.AddSingleton<CalibrationService>();
var app = builder.Build();

// Route 1: อ่านข้อมูล Telemetry (GET)
app.MapGet("/api/telemetry", (CalibrationService cal) =>
{
    int simulatedRaw = 2048; // จำลองค่า ADC จากเซนเซอร์
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

// Route 2: ปรับเทียบเซนเซอร์ (POST)
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

// Route 3: สั่งข้อความขึ้นหน้าจอ OLED (POST)
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