using System.Reactive.Concurrency;
using System.Reactive.Linq;
using Monitor.Services;

namespace Monitor.Workers;

public class WatchdogApp : AEnabledWorker
{
    private readonly SerialMessageService _serialMessageService;
    private readonly SystemService _systemService;
    private readonly IScheduler _scheduler;
    
    public WatchdogApp(ILogger<WatchdogApp> logger, IServiceProvider serviceProvider) : base(logger, serviceProvider)
    {
        _serialMessageService = Services.GetRequiredService<SerialMessageService>();
        _systemService = Services.GetRequiredService<SystemService>();
        _scheduler = Services.GetRequiredService<IScheduler>();
    }

    private void FeedDog()
    {
        Logger.LogInformation("Feed dog. IsShutdownAsked : {isShutdownAsked}", _systemService.IsShutdownAsked());

        _serialMessageService.SetWatchdog(TimeSpan.FromSeconds(10));
    }

    protected override Task Start()
    {
        AddDisposable(_scheduler.SchedulePeriodic(TimeSpan.FromSeconds(5), () =>
        {
            if (!_systemService.IsShutdownAsked())
            {
                FeedDog();
            }
        }));

        return Task.CompletedTask;
    }

    protected override Task Stop()
    {
        if (!_systemService.IsShutdownAsked())
        {
            Logger.LogInformation("Disable watchdog");

            _serialMessageService.SetWatchdog(TimeSpan.Zero);
        }
        else
        {
            Logger.LogInformation("We do not disable watchdog because shutdown asked");
        }
        
        return base.Stop();
    }
}