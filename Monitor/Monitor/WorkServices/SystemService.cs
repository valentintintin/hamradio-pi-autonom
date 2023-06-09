using System.Text.Json;
using Monitor.Extensions;
using Monitor.Models;
using NetDaemon.HassModel.Entities;

namespace Monitor.WorkServices;

public class SystemService : AService
{
    private readonly string? _shutdownFilePath;
    private readonly string? _infoFilePath;
    private readonly string? _timeFilePath;

    public SystemService(ILogger<SystemService> logger, IConfiguration configuration) : base(logger)
    {
        IConfigurationSection configurationSection = configuration.GetSection("System");
        
        _shutdownFilePath = configurationSection.GetValueOrThrow<string>("ShutdownFile");
        _infoFilePath = configurationSection.GetValue<string>("InfoFile");
        _timeFilePath = configurationSection.GetValueOrThrow<string>("TimeFile");

        Logger.LogInformation("Shutdown file is in {path}", _shutdownFilePath);
        Logger.LogInformation("Info file is in {path}", _infoFilePath);
        Logger.LogInformation("Time file is in {path}", _timeFilePath);

        File.WriteAllText(_shutdownFilePath, "0");
        File.Delete(_timeFilePath);
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

    public TimeSpan GetUptime()
    {
        string? uptimeString = EntitiesManagerService.Entities.Uptime?.State;
        
        if (!DateTime.TryParse(uptimeString, out DateTime uptime))
        {
            return TimeSpan.Zero;
        }
        
        return DateTime.UtcNow - uptime;
    }

    public void Shutdown()
    {
        Logger.LogInformation("Send shutdown command");

        if (WillShutdown())
        {
            Logger.LogWarning("Will already shutdown");
            return;
        }
         
        try
        {
            File.WriteAllText(_shutdownFilePath!, "1");
        }
        catch (Exception e)
        {
            Logger.LogError(e, "Send shutdown command error");
        }
    }

    public void SetTime(DateTime dateTime)
    {
        if ((DateTime.UtcNow - dateTime).TotalSeconds <= 10)
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
            File.WriteAllText(_timeFilePath!, dateTime.ToString("s"));
        }
        catch (Exception e)
        {
            Logger.LogError(e, "Change dateTime error");
        }
    }

    private bool WillShutdown()
    {
        try
        {
            return File.ReadAllText(_shutdownFilePath!) == "1";
        }
        catch (Exception e)
        {
            Logger.LogError(e, "Read shutdown file error");
            return false;
        }
    }
}