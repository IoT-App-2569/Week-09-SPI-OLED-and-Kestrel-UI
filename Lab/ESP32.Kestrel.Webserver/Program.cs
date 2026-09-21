using ESP32.Kestrel.Webserver.Services;

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddSingleton<CalibrationService>();
builder.Services.AddSingleton<SerialBridgeService>();
builder.Services.AddHostedService(sp => sp.GetRequiredService<SerialBridgeService>());

var app = builder.Build();

app.UseDefaultFiles();
app.UseStaticFiles();

app.MapGet("/api/telemetry", (CalibrationService cal, SerialBridgeService bridge) =>
{
    return Results.Ok(new
    {
        raw = bridge.LatestRaw,
        calibrated = bridge.LatestCalibrated,
        unit = cal.Settings.Unit,
        displayMsg = cal.CurrentOledMessage,
        isConnected = bridge.IsConnected,
        kestrelLatencyMs = bridge.LastRoundTripLatencyMs,
        timestamp = DateTime.UtcNow
    });
});

app.MapPost("/api/potentiometer/calibrate", (CalibrationSettings newSettings, CalibrationService cal) =>
{
    try
    {
        cal.UpdateSettings(newSettings);
        cal.SetOledMessage("CAL OK");
        return Results.Ok(new { status = "success", settings = cal.Settings });
    }
    catch (ArgumentException ex)
    {
        return Results.BadRequest(new { status = "error", message = ex.Message });
    }
});

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