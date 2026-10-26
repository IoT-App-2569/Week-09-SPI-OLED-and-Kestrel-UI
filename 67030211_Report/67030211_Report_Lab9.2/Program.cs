var builder = WebApplication.CreateBuilder(args);
builder.WebHost.ConfigureKestrel(options => options.ListenAnyIP(5000));
var app = builder.Build();

app.MapPost("/api/oled/calibrate", (CalibrationDto dto) => {
    double calibratedVoltage = dto.Raw * (3.3 / 4095.0) * dto.Gain + dto.Offset;
    return Results.Ok(new { calibratedVoltage, raw = dto.Raw });
});

app.Run();
public record CalibrationDto(int Raw, double Gain, double Offset);
