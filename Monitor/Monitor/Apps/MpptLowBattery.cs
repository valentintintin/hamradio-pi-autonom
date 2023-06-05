using System.Reactive.Linq;
using Monitor.Extensions;
using Monitor.WorkServices;
using NetDaemon.Common;
using NetDaemon.HassModel;

namespace Monitor.Apps;

[NetDaemonApp(Id = "mppt_low_battery_app")]
public class MpptLowBattery : AApp
{
    public MpptLowBattery(IHaContext ha, ILogger<MpptLowBattery> logger, EntitiesManagerService entitiesManagerService) : base(ha, logger, entitiesManagerService)
    {
        EntitiesManagerService.Entities.MpptBatteryVoltage.StateChanges(logger)
            .Select(s => s.Entity.State.ToInt())
            .Average()
            .Sample(TimeSpan.FromMinutes(5))
            .Where(s => s < EntitiesManagerService.Entities.MpptLowBatteryVoltage.State.ToInt())
            .SubscribeAsync(async _ =>
            {
                Logger.LogInformation("Battery is too low so sleep");

                TimeSpan timeSleep = TimeSpan.FromMinutes(EntitiesManagerService.Entities.MpptLowBatteryTimeOff.State.ToInt(30));

                entitiesManagerService.Update(EntitiesManagerService.Entities.TimeSleep, timeSleep.TotalMinutes);
                await entitiesManagerService.UpdateEntities();
            });
    }
}