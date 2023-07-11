using System.Reactive.Linq;
using Monitor.Services;

namespace Monitor.Workers;

public class WatchdogApp : AWorker
{
    private readonly SerialMessageService _serialMessageService;
    private readonly SystemService _systemService;
    
    public WatchdogApp(ILogger<WatchdogApp> logger, IServiceProvider serviceProvider) : base(logger, serviceProvider)
    {
        _serialMessageService = Services.GetRequiredService<SerialMessageService>();
        _systemService = Services.GetRequiredService<SystemService>();
    }

    private void FeedDog()
    {
        Logger.LogInformation("Feed dog");

        _serialMessageService.SetWatchdog(TimeSpan.FromSeconds(10));
    }

    protected override Task Start()
    {
        AddDisposable(EntitiesManagerService.Entities.WatchdogCounter.ValueChanges()
            .Select(v => v.value)
            .Where(s => s.TotalSeconds % 5 == 0)
            .Sample(TimeSpan.FromSeconds(5))
            .Where(_ => !_systemService.IsShutdownAsked())
            .Subscribe(_ =>
            {
                FeedDog();
            })
        );

        Logger.LogInformation("Start watchdog");
        FeedDog();
        
        return Task.CompletedTask;
    }

    protected override Task Stop()
    {
        if (!_systemService.IsShutdownAsked())
        {
            Logger.LogInformation("Disable watchdog");

            _serialMessageService.SetWatchdog(TimeSpan.Zero);
        }
        
        return base.Stop();
    }
}