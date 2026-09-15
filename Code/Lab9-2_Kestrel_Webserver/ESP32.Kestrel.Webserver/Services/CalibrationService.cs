namespace ESP32.Kestrel.Webserver.Services;

/// <summary>
/// พารามิเตอร์การปรับเทียบเซนเซอร์แบบสองจุด (Two-Point Linear Calibration)
/// </summary>
public class CalibrationSettings
{
    public int RawMin { get; set; } = 150;      // ค่าดิบต่ำสุด (Zero Point)
    public int RawMax { get; set; } = 3950;     // ค่าดิบสูงสุด (Span Point)
    public double ScaleMin { get; set; } = 0.0;
    public double ScaleMax { get; set; } = 100.0;
    public string Unit { get; set; } = "%";
}

/// <summary>
/// เอนจินปรับเทียบเซนเซอร์ + ที่เก็บข้อความที่จะส่งไปแสดงบนจอ OLED
/// ลงทะเบียนเป็น Singleton จึงต้องป้องกัน Race Condition ด้วย lock
/// </summary>
public class CalibrationService
{
    private readonly object _gate = new();
    private CalibrationSettings _settings = new();
    private string _currentOledMessage = "SYSTEM READY";

    public CalibrationSettings Settings
    {
        get { lock (_gate) { return _settings; } }
    }

    public string CurrentOledMessage
    {
        get { lock (_gate) { return _currentOledMessage; } }
    }

    public void UpdateSettings(CalibrationSettings newSettings)
    {
        ArgumentNullException.ThrowIfNull(newSettings);

        // ป้องกันข้อผิดพลาดการหารด้วยศูนย์ (Divide-by-Zero Guard)
        if (newSettings.RawMax <= newSettings.RawMin)
        {
            throw new ArgumentException("RawMax ต้องมีค่ามากกว่า RawMin เสมอ!");
        }

        if (string.IsNullOrWhiteSpace(newSettings.Unit))
        {
            newSettings.Unit = "%";
        }

        lock (_gate) { _settings = newSettings; }
    }

    public void SetOledMessage(string msg)
    {
        msg ??= string.Empty;
        // จอกว้าง 21 ตัวอักษร (128 / 6) จึงตัดที่ 20 ตัวอักษรเพื่อความปลอดภัย
        var trimmed = msg.Length > 20 ? msg[..20] : msg;
        lock (_gate) { _currentOledMessage = trimmed; }
    }

    /// <summary>
    /// สูตร: V = (clamp(raw) - RawMin) / (RawMax - RawMin) * (ScaleMax - ScaleMin) + ScaleMin
    /// </summary>
    public double Compute(int rawAdc)
    {
        var s = Settings;

        // Clamp ค่าให้อยู่ในช่วงที่กำหนด ป้องกันสเกลทะลัก (Out-of-Bounds Clamping)
        int clamped = Math.Clamp(rawAdc, s.RawMin, s.RawMax);

        return ((double)(clamped - s.RawMin) / (s.RawMax - s.RawMin))
               * (s.ScaleMax - s.ScaleMin) + s.ScaleMin;
    }
}
