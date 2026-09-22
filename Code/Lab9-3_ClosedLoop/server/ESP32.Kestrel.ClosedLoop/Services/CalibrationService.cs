namespace ESP32.Kestrel.ClosedLoop.Services;

public class CalibrationSettings
{
    public int RawMin { get; set; } = 150;
    public int RawMax { get; set; } = 3950;
    public double ScaleMin { get; set; } = 0.0;
    public double ScaleMax { get; set; } = 100.0;
    public string Unit { get; set; } = "%";
}

/// <summary>
/// เอนจินปรับเทียบตามที่กำหนดในใบงาน 9.3 (ขั้นที่ 3.2.2)
///
/// หมายเหตุการแก้ไข: ต่างจากโค้ดต้นฉบับในใบงานตรงที่เพิ่ม `lock` เข้ามาป้องกัน
/// Race Condition — เพราะใน Lab 9.3 ออบเจกต์นี้ถูกลงทะเบียนเป็น Singleton และถูกแตะ
/// จาก 2 เธรดพร้อมกันจริง: (1) HTTP Request thread เวลาเรียก /api/potentiometer/calibrate
/// หรือ /api/oled/message และ (2) SerialBridgeService ซึ่งเป็น BackgroundService ที่วน
/// อ่าน/เขียนค่าทุก ๆ รอบ Serial (สูงสุด ~20 Hz) พร้อมกันตลอดเวลาที่เซิร์ฟเวอร์ทำงาน
/// ถ้าไม่ล็อก อาจเกิด Torn Read (อ่านเจอ object ที่เขียนไม่เสร็จ) ได้ในทางทฤษฎี
/// </summary>
public class CalibrationService
{
    private readonly object _gate = new();
    private CalibrationSettings _settings = new();
    private string _currentOledMessage = "READY";

    public CalibrationSettings Settings { get { lock (_gate) return _settings; } }
    public string CurrentOledMessage { get { lock (_gate) return _currentOledMessage; } }

    public void UpdateSettings(CalibrationSettings newSettings)
    {
        if (newSettings.RawMax <= newSettings.RawMin)
            throw new ArgumentException("RawMax ต้องมีค่ามากกว่า RawMin เสมอ!");

        lock (_gate) _settings = newSettings;
    }

    public void SetOledMessage(string msg)
    {
        msg ??= string.Empty;
        // จอกว้าง 128px, Zone 3 ข้อความเริ่มที่ x=42 เหลือที่ ~86px = 14 ตัวอักษร
        // ตั้ง limit ไว้ที่ 16 ตามใบงาน (oled_draw_string() จะตัดของจริงที่ขอบจออีกชั้นหนึ่ง)
        var trimmed = msg.Length > 16 ? msg[..16] : msg;
        lock (_gate) _currentOledMessage = trimmed;
    }

    public double Compute(int rawAdc)
    {
        var s = Settings;
        int clamped = Math.Clamp(rawAdc, s.RawMin, s.RawMax);
        return ((double)(clamped - s.RawMin) / (s.RawMax - s.RawMin))
               * (s.ScaleMax - s.ScaleMin) + s.ScaleMin;
    }
}
