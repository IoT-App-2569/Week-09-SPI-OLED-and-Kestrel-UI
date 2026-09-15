namespace ESP32.Kestrel.Webserver.Services;

public class CalibrationSettings
{
    public int RawMin { get; set; } = 150;
    public int RawMax { get; set; } = 3950;
    public double ScaleMin { get; set; } = 0.0;
    public double ScaleMax { get; set; } = 100.0;
    public string Unit { get; set; } = "%";
}

public class CalibrationService
{
    private readonly object _sync = new();
    private CalibrationSettings _settings = new();
    private string _currentOledMessage = "SYSTEM READY";
    private int _latestRawAdc = 2048;
    private DateTime _latestTimestamp = DateTime.UtcNow;

    public CalibrationSettings Settings { get { lock (_sync) return _settings; } }
    public string CurrentOledMessage { get { lock (_sync) return _currentOledMessage; } }
    public int LatestRawAdc { get { lock (_sync) return _latestRawAdc; } }
    public DateTime LatestTimestamp { get { lock (_sync) return _latestTimestamp; } }

    public void UpdateSettings(CalibrationSettings newSettings)
    {
        if (newSettings.RawMax <= newSettings.RawMin)
        {
            throw new ArgumentException("RawMax ต้องมีค่ามากกว่า RawMin เสมอ!");
        }

        lock (_sync) _settings = newSettings;
    }

    public void SetOledMessage(string message)
    {
        lock (_sync) _currentOledMessage = message.Length > 20 ? message[..20] : message;
    }

    public void UpdateRawAdc(int rawAdc)
    {
        lock (_sync) { _latestRawAdc = rawAdc; _latestTimestamp = DateTime.UtcNow; }
    }

    public double Compute(int rawAdc)
    {
        lock (_sync)
        {
            int clamped = Math.Clamp(rawAdc, _settings.RawMin, _settings.RawMax);
            return ((double)(clamped - _settings.RawMin) / (_settings.RawMax - _settings.RawMin))
                   * (_settings.ScaleMax - _settings.ScaleMin) + _settings.ScaleMin;
        }
    }
}
