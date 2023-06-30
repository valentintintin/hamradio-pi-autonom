using System.Reactive.Linq;
using Monitor.Extensions;
using Monitor.Services;
using NetDaemon.AppModel;
using NetDaemon.HassModel;

namespace Monitor.Apps;

[NetDaemonApp(Id = "mppt_watchdog_app")]
public class MpptWatchdogApp : AApp, IAsyncDisposable
{
    private readonly SerialMessageService _serialMessageService;
    private readonly IDisposable _feeder;
    
    public MpptWatchdogApp(IHaContext ha, ILogger<MpptWatchdogApp> logger, EntitiesManagerService entitiesManagerService,
        SerialMessageService serialMessageService) : base(ha, logger, entitiesManagerService)
    {
        _serialMessageService = serialMessageService;

        _feeder = EntitiesManagerService.Entities.MpptWatchdogCounter.StateChanges(logger)
            .Where(s => s.DiffState(logger) < 0)
            .Sample(TimeSpan.FromSeconds(5))
            .Where(_ => EntitiesManagerService.Entities.TimeSleep.IsOff(logger))
            .Subscribe(_ =>
            {
                FeedDog();
            });

        Logger.LogInformation("Start watchdog");
        FeedDog();
    }

    private void FeedDog()
    {
        Logger.LogInformation("Feed dog");

        _serialMessageService.SetWatchdog(TimeSpan.FromSeconds(10));
    }

    public ValueTask DisposeAsync()
    {
        _feeder.Dispose();

        if (EntitiesManagerService.Entities.TimeSleep.IsOff(Logger))
        {
            Logger.LogInformation("Disable watchdog");

            _serialMessageService.SetWatchdog(TimeSpan.Zero);
        }

        GC.SuppressFinalize(this);
        
        return ValueTask.CompletedTask;
    }
}