using System.Globalization;
using System.Reactive.Subjects;
using System.Text.Json;
using Monitor.Extensions;
using Monitor.Models;

namespace Monitor.Services;

public class SystemService : AService
{
    private readonly SerialMessageService _serialMessageService;
    private readonly string _shutdownFilePath;
    private readonly string _timeFilePath;
    private readonly string? _infoFilePath;
    private TimeSpan? WillSleep { get; set; }

    public SystemService(ILogger<SystemService> logger, IConfiguration configuration, SerialMessageService serialMessageService) : base(logger)
    {
        _serialMessageService = serialMessageService;
        
        IConfigurationSection configurationSection = configuration.GetSection("System");
        
        _shutdownFilePath = configurationSection.GetValueOrThrow<string>("ShutdownFile");
        _timeFilePath = configurationSection.GetValueOrThrow<string>("TimeFile");
        _infoFilePath = configurationSection.GetValue<string>("InfoFile");
    }

    public SystemState? GetInfo()
    {
        if (string.IsNullOrWhiteSpace(_infoFilePath))
        {
            Logger.LogWarning("Get system without info file path");
            
            return null;
        }

        try
        {
            string infoJson = File.ReadAllText(_infoFilePath!);
            Logger.LogInformation("System info : {json}", infoJson);
            return JsonSerializer.Deserialize<SystemState>(infoJson);
        }
        catch (Exception e)
        {
            Logger.LogError(e, "System info reading error");
            return null;
        }
    }

    public void AskForShutdown(TimeSpan sleepTime)
    {
        Logger.LogInformation("Ask for shutdown during {time}", sleepTime);
        
        if (IsShutdownOrderGiven())
        {
            Logger.LogWarning("Ask for shutdown during {time} KO because already shutdown order given", sleepTime);
            
            return;
        }
        
        WillSleep = sleepTime;

        if (WillSleep.HasValue)
        {
            _serialMessageService.SetWatchdog(WillSleep.Value);
        }
    }

    public void Shutdown()
    {
        Logger.LogInformation("Send shutdown command");

        if (IsShutdownOrderGiven())
        {
            Logger.LogWarning("Will already shutdown");
            return;
        }
        
        try
        {
            File.WriteAllText(_shutdownFilePath, "1");
        }
        catch (Exception e)
        {
            Logger.LogError(e, "Send shutdown command error");
        }
    }

    public void SetTime(DateTime dateTime)
    {
        if (Math.Abs((DateTime.UtcNow - dateTime).TotalSeconds) <= 10)
        {
            Logger.LogDebug("Change dateTime not done because difference is < 10 second : {now} and {new}", DateTime.UtcNow, dateTime);
            return;
        }
        
        if (dateTime.Year != 2023)
        {
            Logger.LogWarning("Change dateTime impossible because incoherent {dateTime}", dateTime);
            return;
        }
        
        Logger.LogInformation("Change dateTime {dateTime}", dateTime);

        try
        {
            File.WriteAllText(_timeFilePath, dateTime.ToString("s"));
        }
        catch (Exception e)
        {
            Logger.LogError(e, "Change dateTime error");
        }
    }

    public bool IsShutdownAsked()
    {
        return WillSleep.HasValue || IsShutdownOrderGiven();
    }

    public bool IsShutdownOrderGiven()
    {
        try
        {
            return File.ReadAllText(_shutdownFilePath) == "1";
        }
        catch (Exception e)
        {
            Logger.LogError(e, "Read shutdown file error");
            return false;
        }
    }
}