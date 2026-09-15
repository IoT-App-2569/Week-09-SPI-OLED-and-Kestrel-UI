using System.IO.Ports;

namespace ESP32.Kestrel.Webserver.Services;

public sealed class SerialBridgeService(IConfiguration configuration, CalibrationService calibration,
    ILogger<SerialBridgeService> logger) : BackgroundService
{
    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        // Yield immediately so Kestrel can bind its HTTP port before the
        // synchronous SerialPort.ReadLine loop begins on a worker thread.
        await Task.Yield();

        string name = configuration["Serial:PortName"] ?? "COM3";
        int baud = configuration.GetValue("Serial:BaudRate", 115200);
        while (!stoppingToken.IsCancellationRequested)
        {
            try
            {
                using var port = new SerialPort(name, baud) { NewLine = "\n", ReadTimeout = 500, WriteTimeout = 500 };
                port.Open();
                logger.LogInformation("Serial bridge connected to {PortName}.", name);
                while (!stoppingToken.IsCancellationRequested)
                {
                    try
                    {
                        string line = port.ReadLine().Trim();
                        if (!line.StartsWith("ADC:") || !int.TryParse(line.AsSpan(4), out int raw)) continue;
                        calibration.UpdateRawAdc(raw);
                        int percent = (int)Math.Round(calibration.Compute(raw));
                        string message = calibration.CurrentOledMessage.Replace(' ', '_').Replace(',', '_');
                        port.WriteLine($"{percent},{message}");
                    }
                    catch (TimeoutException) { }
                }
            }
            catch (Exception ex) when (!stoppingToken.IsCancellationRequested)
            {
                logger.LogWarning(ex, "Cannot open {PortName}; retrying.", name);
                await Task.Delay(2000, stoppingToken);
            }
        }
    }
}
