using System.Globalization;
using System.Reactive.Subjects;
using System.Text.Json;
using Monitor.Extensions;
using Monitor.Models;

namespace Monitor.Services;

public class SystemService : AService
{
    private readonly SerialMessageService _serialMessageService;
    public DateTime? ChangeDateTime { get; set; }
    private TimeSpan? WillSleep { get; set; }
    private bool WillShutdown { get; set; }

    public SystemService(ILogger<SystemService> logger, SerialMessageService serialMessageService) : base(logger)
    {
        _serialMessageService = serialMessageService;
    }

    public void AskForShutdown(TimeSpan sleepTime)
    {
        Logger.LogInformation("Ask for shutdown during {time}", sleepTime);
        
        if (WillShutdown)
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

        if (WillShutdown)
        {
            Logger.LogWarning("Will already shutdown");
            return;
        }

        WillShutdown = true;
    }

    public void SetTime(DateTime dateTime)
    {
        if (Math.Abs((DateTime.UtcNow - dateTime).TotalSeconds) <= 20)
        {
            Logger.LogDebug("Change dateTime not done because difference is < 10 second : {now} and {new}", DateTime.UtcNow, dateTime);
            return;
        }
        
        if (dateTime.Year != 2023)
        {
            Logger.LogWarning("Change dateTime impossible because incoherent {dateTime}", dateTime);
            return;
        }
        
        Logger.LogInformation("Change dateTime {dateTime}. Old : {now}", dateTime, DateTime.UtcNow);

        ChangeDateTime = dateTime;
    }
    
    public bool IsShutdownAsked()
    {
        return WillSleep.HasValue || WillShutdown;
    }
}