namespace ESP32.Kestrel.Webserver.Services;

public class CalibrationSettings
{
    public int RawMin { get; set; } = 0;
    public int RawMax { get; set; } = 4095;
    public double ScaleMin { get; set; } = 0.0;
    public double ScaleMax { get; set; } = 100.0;
    public string Unit { get; set; } = "%";
}

public class CalibrationService
{
    private CalibrationSettings _settings = new();
    private string _currentOledMessage = "READY";

    public CalibrationSettings Settings => _settings;
    public string CurrentOledMessage => _currentOledMessage;

    public void UpdateSettings(CalibrationSettings newSettings)
    {
        if (newSettings.RawMax <= newSettings.RawMin)
            throw new ArgumentException("RawMax ต้องมีค่ามากกว่า RawMin เสมอ!");
        _settings = newSettings;
    }

    public void SetOledMessage(string msg)
    {
        _currentOledMessage = msg.Length > 16 ? msg[..16] : msg;
    }

    public double Compute(int rawAdc)
    {
        int clamped = Math.Clamp(rawAdc, _settings.RawMin, _settings.RawMax);
        return ((double)(clamped - _settings.RawMin) / (_settings.RawMax - _settings.RawMin))
               * (_settings.ScaleMax - _settings.ScaleMin) + _settings.ScaleMin;
    }
}