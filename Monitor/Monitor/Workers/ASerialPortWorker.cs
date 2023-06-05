using System.IO.Ports;
using Monitor.Extensions;
using Timer = System.Timers.Timer;

namespace Monitor.Workers;

public abstract class ASerialPortWorker : AWorker
{
    protected SerialPort? SerialPort;

    private readonly string _path;
    private readonly int _speed;
    private readonly string _fakeInput;

    public ASerialPortWorker(string configSectionName, ILogger<ASerialPortWorker> logger, IConfiguration configuration, 
        IServiceScopeFactory serviceScopeFactory,
        string fakeInput = "") : base(configSectionName, logger, configuration, serviceScopeFactory)
    {
        _fakeInput = fakeInput;

        _path = ConfigurationSection.GetValueOrThrow<string>("Path");
        _speed = ConfigurationSection.GetValueOrThrow<int>("Speed");
    }

    protected abstract Task MessageReceived(string input);
    
    protected override Task Start()
    {
        if (ConfigurationSection.GetValue<bool?>("Simulate") == true)
        {
            Timer timer = new(2500);

            string[] lines = _fakeInput.Split('\n');
            int currentLine = 0;

            timer.Elapsed += (_, _) =>
            {
                string input = lines[currentLine++];
                
                Logger.LogTrace("Received serial message : {input}", input);
                
                MessageReceived(input);
                
                if (currentLine >= lines.Length)
                {
                    currentLine = 0;
                }
            };
            timer.Start();
        }
        else
        {
            SerialPort = new SerialPort(_path, _speed);
            SerialPort.ReadTimeout = 1000;
            SerialPort.DataReceived += (_, _) =>
            {
                try
                {
                    string input = SerialPort.ReadLine();
                
                    Logger.LogTrace("Received serial : {input}", input);

                    MessageReceived(input);
                }
                catch (Exception e)
                {
                    Logger.LogError(e, "SerialPort error during read or decode");
                }
            };
            
            try
            {
                SerialPort.Open();
                
                Logger.LogInformation("SerialPort {path}:{speed} opened OK", _path, _speed);
            }
            catch (Exception e)
            {
                Logger.LogError(e, "SerialPort {path}:{speed} opened KO", _path, _speed);
            }
        }

        return Task.CompletedTask;
    }

    public override void Dispose()
    {
        SerialPort?.Dispose();

        base.Dispose();
    }
}