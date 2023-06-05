using System.Reactive.Linq;
using Monitor.Extensions;
using Monitor.WorkServices;
using NetDaemon.AppModel;
using NetDaemon.HassModel;

namespace Monitor.Apps;

[NetDaemonApp(Id = "mppt_watchdog_app")]
public class MpptWatchdogApp : AApp, IAsyncDisposable
{
    private readonly SerialMessageService _serialMessageService;

    public MpptWatchdogApp(IHaContext ha, ILogger<MpptWatchdogApp> logger, EntitiesManagerService entitiesManagerService,
        SerialMessageService serialMessageService) : base(ha, logger, entitiesManagerService)
    {
        _serialMessageService = serialMessageService;
        
        EntitiesManagerService.Entities.MpptWatchdogCounter.StateChanges(logger)
            .Where(s => EntitiesManagerService.Entities.TimeSleep.IsOff(logger))
            .Where(s => s.DiffState(logger) < 0)
            .Sample(TimeSpan.FromSeconds(5))
            .Subscribe(_ =>
            {
                FeedDog();
            });

        EntitiesManagerService.Entities.MpptWatchdog.StateChanges(logger)
            .Where(s => EntitiesManagerService.Entities.TimeSleep.IsOff(logger))
            .SubscribeAsync(async s =>
            {
                if (s.Entity.IsOn())
                {
                    Logger.LogTrace("Restart watchdog");

                    FeedDog();
                }
                else
                {
                    Logger.LogTrace("Stop watchdog");
                    
                    _serialMessageService.SetWatchdog(TimeSpan.Zero);
                }
                
                entitiesManagerService.Update(EntitiesManagerService.Entities.TimeSleep, "0");
                await entitiesManagerService.UpdateEntities();
            });
    }

    private void FeedDog()
    {
        Logger.LogTrace("Feed dog");

        _serialMessageService.SetWatchdog(TimeSpan.FromSeconds(255));
    }

    public ValueTask DisposeAsync()
    {
        if (EntitiesManagerService.Entities.TimeSleep.IsOff(Logger))
        {
            Logger.LogInformation("Disable watchdog");

            _serialMessageService.SetWatchdog(TimeSpan.Zero);
        }

        return ValueTask.CompletedTask;
    }
}