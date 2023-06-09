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
            .Buffer(TimeSpan.FromMinutes(2))
            .Select(s => s.Average())
            .Do(s => Logger.LogDebug("Average Battery Voltage : {averageVoltage}", s))
            .Where(s => s < EntitiesManagerService.Entities.MpptLowBatteryVoltage.State.ToInt())
            .SubscribeAsync(async s =>
            {
                Logger.LogWarning("Battery is too low so sleep. {batteryVoltage} < {lowVoltage}", s, EntitiesManagerService.Entities.MpptLowBatteryVoltage.State);

                TimeSpan timeSleep = TimeSpan.FromMinutes(EntitiesManagerService.Entities.MpptLowBatteryTimeOff.State.ToInt(30));
                
                entitiesManagerService.Update(EntitiesManagerService.Entities.TimeSleep, timeSleep.TotalMinutes);
                await entitiesManagerService.UpdateEntities();
            });
    }
}