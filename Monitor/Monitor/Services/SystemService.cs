using System.Text.Json;
using Monitor.Extensions;
using Monitor.Models;

namespace Monitor.Services;

public class SystemService : AService
{
    private readonly string? _shutdownFilePath;
    private readonly string? _infoFilePath;
    private readonly string? _timeFilePath;
    private readonly bool _enabled;
    
    public SystemService(ILogger<SystemService> logger, IConfiguration configuration) : base(logger)
    {
        IConfigurationSection configurationSection = configuration.GetSection("System");
        
        _enabled = configurationSection.GetValueOrThrow<bool>("Enabled");

        if (!_enabled) return;
        
        _shutdownFilePath = configurationSection.GetValueOrThrow<string>("ShutdownFile");
        _infoFilePath = configurationSection.GetValueOrThrow<string>("InfoFile");
        _timeFilePath = configurationSection.GetValueOrThrow<string>("TimeFile");

        File.WriteAllText(_shutdownFilePath, "0");
        File.Delete(_timeFilePath);
    }

    public SystemState? GetInfo()
    {
        if (!_enabled) return null;

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

    public void Shutdown()
    {
        if (!_enabled) return;
        
        if (WillShutdown())
        {
            Logger.LogInformation("Will shutdown");
        }
        else
        {
            Logger.LogInformation("Send shutdown order");

            try
            {
                File.WriteAllText(_shutdownFilePath!, "1");
            }
            catch (Exception e)
            {
                Logger.LogError(e, "Send shutdown order error");
            }
        }
    }

    public bool WillShutdown()
    {
        if (!_enabled) return false;

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

    public void SetTime(DateTime dateTime)
    {
        if (!_enabled) return;
        
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
}