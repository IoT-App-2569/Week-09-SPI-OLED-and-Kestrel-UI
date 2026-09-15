using ESP32.Kestrel.Webserver.Services;

var builder = WebApplication.CreateBuilder(args);
builder.Services.AddSingleton<CalibrationService>();
builder.Services.AddHostedService<SerialBridgeService>();

var app = builder.Build();

app.MapGet("/", () => Results.Content("""
<!doctype html><title>ESP32 Telemetry</title><style>body{font-family:Arial;max-width:620px;margin:40px auto}svg{width:100%;height:90px}#bar{fill:#22c55e}pre{background:#111;color:#eee;padding:16px}</style>
<h1>ESP32 Telemetry</h1><svg viewBox="0 0 100 10"><rect width="100" height="10" fill="#ddd"/><rect id="bar" width="0" height="10"/></svg><h2 id="value">-- %</h2><pre id="details">Waiting for ESP32...</pre><script>async function read(){let t=await fetch('/api/telemetry').then(r=>r.json());bar.setAttribute('width',t.calibrated);value.textContent=t.calibrated+t.unit;details.textContent=JSON.stringify(t,null,2)}read();setInterval(read,250)</script>
""", "text/html"));

app.MapGet("/api/telemetry", (CalibrationService calibration) =>
{
    int simulatedRaw = calibration.LatestRawAdc;
    double calibrated = calibration.Compute(simulatedRaw);

    return Results.Ok(new
    {
        raw = simulatedRaw,
        calibrated = Math.Round(calibrated, 1),
        unit = calibration.Settings.Unit,
        displayMsg = calibration.CurrentOledMessage,
        timestamp = calibration.LatestTimestamp,
    });
});

app.MapPost("/api/potentiometer/calibrate", (CalibrationSettings newSettings,
                                                CalibrationService calibration) =>
{
    try
    {
        calibration.UpdateSettings(newSettings);
        calibration.SetOledMessage("CALIBRATED OK");
        return Results.Ok(new { status = "success", settings = calibration.Settings });
    }
    catch (ArgumentException exception)
    {
        return Results.BadRequest(new { status = "error", message = exception.Message });
    }
});

app.MapPost("/api/oled/message", (DisplayMessageRequest request,
                                    CalibrationService calibration) =>
{
    if (string.IsNullOrWhiteSpace(request.Message))
    {
        return Results.BadRequest(new { status = "error", message = "ข้อความต้องไม่ว่างเปล่า" });
    }

    calibration.SetOledMessage(request.Message);
    return Results.Ok(new { status = "success", current = calibration.CurrentOledMessage });
});

app.Run();
