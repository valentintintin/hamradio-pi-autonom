using System.IO.Ports;
using System.Reactive.Concurrency;
using Monitor.Extensions;
using Monitor.Services;
using NetDaemon.HassModel;

namespace Monitor.Apps;

public abstract class ASerialPortApp : AApp, IAsyncDisposable
{
    protected readonly MonitorService MonitorService;
    protected readonly SerialPort? SerialPort;

    protected ASerialPortApp(IHaContext ha, ILogger<ASerialPortApp> logger, EntitiesManagerService entitiesManagerService,
        MonitorService monitorService, IConfiguration configuration, IScheduler scheduler,
        string configSectionName, string fakeInput = "")
        : base(ha, logger, entitiesManagerService)
    {
        MonitorService = monitorService;
        IConfigurationSection configurationSection = configuration.GetSection(configSectionName);

        string path = configurationSection.GetValueOrThrow<string>("Path");
        int speed = configurationSection.GetValueOrThrow<int>("Speed");
        
        if (configurationSection.GetValue<bool?>("Simulate") == true)
        {
            string[] lines = fakeInput.Split('\n');
            int currentLine = 0;

            scheduler.SchedulePeriodic(TimeSpan.FromMilliseconds(1500), () => 
            {
                string input = lines[currentLine++];
                
                Logger.LogTrace("Received serial message : {input}", input);
                
                MessageReceived(input);
                
                if (currentLine >= lines.Length)
                {
                    currentLine = 0;
                }
            });
        }
        else
        {
            SerialPort = new SerialPort(path, speed);
            SerialPort.NewLine = "\n";
            SerialPort.DataReceived += (_, _) =>
            {
                try
                {
                    string input = SerialPort.ReadLine();
                
                    Logger.LogDebug("Received serial : {input}", input);

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
                
                Logger.LogInformation("SerialPort {path}:{speed} opened OK", path, speed);
            }
            catch (Exception e)
            {
                Logger.LogCritical(e, "SerialPort {path}:{speed} opened KO", path, speed);
            }
        }
    }

    protected abstract Task MessageReceived(string input);
    
    public ValueTask DisposeAsync()
    {
        SerialPort?.Close();

        GC.SuppressFinalize(this);

        return ValueTask.CompletedTask;
    }
}