var builder = WebApplication.CreateBuilder(args);
var app = builder.Build();
app.MapPost("/api/oled/calibrate", (CalDto dto) => Results.Ok(new { v = dto.Raw * 3.3 / 4095.0 }));
app.Run();
public record CalDto(int Raw);
