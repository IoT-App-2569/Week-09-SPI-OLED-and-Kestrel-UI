using System.IO.Ports;
using System.Diagnostics;

namespace ESP32.Kestrel.Webserver.Services;

public class SerialBridgeService : BackgroundService
{
    private readonly ILogger<SerialBridgeService> _logger;
    private readonly CalibrationService _calibrationService;
    private readonly IConfiguration _config;
    private SerialPort? _serialPort;

    public int LatestRaw { get; private set; } = 0;
    public double LatestCalibrated { get; private set; } = 0.0;
    public long LatestEspUptime { get; private set; } = 0;
    public long LastRoundTripLatencyMs { get; private set; } = 0;
    public bool IsConnected => _serialPort?.IsOpen ?? false;

    public SerialBridgeService(ILogger<SerialBridgeService> logger, 
                               CalibrationService calService, 
                               IConfiguration config)
    {
        _logger = logger;
        _calibrationService = calService;
        _config = config;
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        // TODO: เปลี่ยน COM24 เป็นพอร์ตของ ESP32 คุณ
        string portName = _config["SerialPort:PortName"] ?? "COM3"; 
        int baudRate = 115200;

        _logger.LogInformation("กำลังเปิดการเชื่อมต่อ Serial Port: {Port} ที่ BaudRate {Baud}", portName, baudRate);

        while (!stoppingToken.IsCancellationRequested)
        {
            try
            {
                if (_serialPort == null || !_serialPort.IsOpen)
                {
                    _serialPort = new SerialPort(portName, baudRate)
                    {
                        NewLine = "\n",
                        ReadTimeout = 2000,
                        WriteTimeout = 500
                    };
                    _serialPort.Open();
                    _logger.LogInformation("เชื่อมต่อพอร์ต {Port} สำเร็จ!", portName);
                }

                string rawLine = _serialPort.ReadLine().Trim();
                long receiveTime = Stopwatch.GetTimestamp();

                if (rawLine.StartsWith("ADC:"))
                {
                    string[] parts = rawLine[4..].Split(',');
                    if (int.TryParse(parts[0], out int rawValue))
                    {
                        LatestRaw = rawValue;
                        if (parts.Length > 1 && long.TryParse(parts[1], out long espUptime))
                        {
                            LatestEspUptime = espUptime;
                        }

                        LatestCalibrated = Math.Round(_calibrationService.Compute(rawValue), 1);

                        int percentInt = (int)Math.Round(LatestCalibrated);
                        string msg = _calibrationService.CurrentOledMessage;
                        string txCommand = $"SET:{percentInt}:{msg}\n";

                        _serialPort.Write(txCommand);

                        long elapsedNanos = Stopwatch.GetElapsedTime(receiveTime).Ticks * 100;
                        LastRoundTripLatencyMs = elapsedNanos / 1_000_000;
                    }
                }
            }
            catch (TimeoutException) { }
            catch (Exception ex)
            {
                _logger.LogWarning("เกิดข้อผิดพลาดในการสื่อสาร Serial: {Msg}. กำลังลองเชื่อมต่อใหม่ใน 2 วินาที...", ex.Message);
                _serialPort?.Dispose();
                _serialPort = null;
                await Task.Delay(2000, stoppingToken);
            }
            await Task.Yield();
        }
        if (_serialPort?.IsOpen == true) _serialPort.Close();
    }
}