using System.Reactive.Linq;
using Monitor.Extensions;
using Monitor.WorkServices;
using NetDaemon.Common;
using NetDaemon.HassModel;

namespace Monitor.Apps;

[NetDaemonApp(Id = "sleep_app")]
public class SleepApp : AApp
{
    public SleepApp(IHaContext ha, ILogger<SleepApp> logger, EntitiesManagerService entitiesManagerService,
        SerialMessageService serialMessageService) : base(ha, logger, entitiesManagerService)
    {
        EntitiesManagerService.Entities.TimeSleep.StateChanges(logger)
            .Where(s => !s.Entity.IsOff())
            .Select(s => s.Entity.State.ToInt())
            .Where(s => s > 0)
            .Subscribe(s =>
            {
                TimeSpan sleepTime = TimeSpan.FromMinutes(s);

                Logger.LogInformation("Sleep during {sleepTime} => {wakeUpTime}", sleepTime, DateTime.UtcNow.Add(sleepTime));

                serialMessageService.SetWatchdog(sleepTime);
            });
    }
}