using System.Reactive.Concurrency;
using Monitor.Services;

namespace Monitor.Workers;

public class SystemInfoApp : AWorker
{
    private readonly MonitorService _monitorService;
    
    public SystemInfoApp(ILogger<SystemInfoApp> logger, IServiceProvider serviceProvider) : base(logger, serviceProvider)
    {
        _monitorService = Services.GetRequiredService<MonitorService>();
    }

    protected override Task Start()
    {
        AddDisposable(Scheduler.SchedulePeriodic(TimeSpan.FromMinutes(1), () => _monitorService.UpdateSystemState()));
        
        return Task.CompletedTask;
    }
}