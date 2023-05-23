using System.IO.Ports;
using Web.Extensions;
using Web.Models.SerialMessages;
using Web.Services;
using Timer = System.Timers.Timer;

namespace Web.Workers;

public class SerialPortWorker : IHostedService, IDisposable
{
    private readonly ILogger<SerialPortWorker> _logger;
    private readonly IConfiguration _configuration;
    private readonly SerialMessageService _serialMessageService;
    private SerialPort? _arduino;

    public SerialPortWorker(ILogger<SerialPortWorker> logger, IConfiguration configuration,
        SerialMessageService serialMessageService)
    {
        _logger = logger;
        _configuration = configuration;
        _serialMessageService = serialMessageService;
    }
    
    public Task StartAsync(CancellationToken cancellationToken)
    {
        Console.WriteLine("Starting");

        IConfigurationSection configurationSection = _configuration.GetSection("SerialPort");

        if (configurationSection.GetValue<bool?>("Simulate") == true)
        {
            Timer timer = new(1000);
            
            string input = @"{""type"":""system"",""state"":""started"",""boxOpened"":false}
     {""type"":""time"",""state"":2313942038,""uptime"":2}
     {""type"":""mppt"",""batteryVoltage"":12268,""batteryCurrent"":2,""solarVoltage"":163,""solarCurrent"":2,""currentCharge"":0,""status"":136,""night"":true,""alert"":false,""watchdogEnabled"":false,""watchdogPowerOffTime"":10,""watchdogCounter"":0,""powerEnabled"":true}
     {""type"":""weather"",""temperature"":20.50,""humidity"":55}
     {""type"":""lora"",""state"":""tx"",""payload"":""F4HVV-15>F4HVV-10,RFONLY:!/7V&-OstcI!!G Solaire camera + NPR70""}
     {""type"":""gpio"",""state"":1,""name"":""wifi"",""pin"":3}";

            string[] lines = input.Split('\n');
            int currentLine = 0;

            timer.Elapsed += (_, _) =>
            {
                _serialMessageService.ParseMessage(lines[currentLine++]);
                
                if (currentLine >= lines.Length)
                {
                    currentLine = 0;
                }
            };
            timer.Start();
        }
        else
        {
            _arduino = new SerialPort(configurationSection.GetValueOrThrow<string>("Path"), configurationSection.GetValueOrThrow<int>("Speed"));
            _arduino.ReadTimeout = 1000;
            _arduino.DataReceived += (_, _) =>
            {
                try
                {
                    string received = _arduino.ReadLine();
                    
                    _logger.LogTrace("SerialPort received new message: {message}", received);

                    if (received.Contains("Copyright"))
                    {
                        return;
                    }

                    _serialMessageService.ParseMessage(received);
                }
                catch (Exception e)
                {
                    _logger.LogError(e, "SerialPort error during read or decode");
                }
            };
            
            try
            {
                _arduino.Open();
            }
            catch (Exception e)
            {
                _logger.LogError(e, "SerialPort error during open");
            }
        }

        Console.WriteLine("Started");

        return Task.CompletedTask;
    }

    public Task StopAsync(CancellationToken cancellationToken)
    {
        return Task.CompletedTask;
    }

    public void Dispose()
    {
        _arduino?.Dispose();
    }
}