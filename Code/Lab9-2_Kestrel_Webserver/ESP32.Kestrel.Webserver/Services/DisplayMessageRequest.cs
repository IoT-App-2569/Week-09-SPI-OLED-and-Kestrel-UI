namespace ESP32.Kestrel.Webserver.Services;

public class DisplayMessageRequest
{
    public string Message { get; set; } = string.Empty;

    /// <summary>กลับสีพื้น/ตัวอักษรบนโซน Footer ของจอ OLED (0xA7 / 0xA6)</summary>
    public bool Invert { get; set; } = false;
}
