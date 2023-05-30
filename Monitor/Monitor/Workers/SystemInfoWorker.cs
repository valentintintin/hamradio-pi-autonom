using Monitor.Extensions;
using Monitor.Models;
using Monitor.WorkServices;
using Timer = System.Timers.Timer;

namespace Monitor.Workers;

public class SystemInfoWorker : AWorker
{
    private readonly SystemService _systemService;
    private readonly Timer _timer;
    
    public SystemInfoWorker(ILogger<SystemInfoWorker> logger, IConfiguration configuration,
        IServiceScopeFactory serviceScopeFactory) : 
        base("System", logger, configuration, serviceScopeFactory)
    {
        _systemService = ServiceProvider.GetRequiredService<SystemService>();
        _timer = new Timer(ConfigurationSection.GetValueOrThrow<int>("Time"));
    }

    protected override Task Start()
    {
        _timer.Elapsed += (_, _) =>
        {
            SystemState? systemState = _systemService.GetInfo();
            
            if (systemState != null)
            {
                MonitorService.UpdateSystemInfo(systemState);
            }
        };
        
        _timer.Start();

        return Task.CompletedTask;
    }

    public override void Dispose()
    {
        _timer.Stop();
        _timer.Dispose();
        base.Dispose();
    }
}