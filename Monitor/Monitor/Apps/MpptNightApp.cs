using System.Reactive.Linq;
using Monitor.Extensions;
using Monitor.WorkServices;
using NetDaemon.AppModel;
using NetDaemon.HassModel;
using NetDaemon.HassModel.Entities;

namespace Monitor.Apps;

[NetDaemonApp(Id = "mppt_night_app")]
public class MpptNightApp : AApp
{
    public MpptNightApp(IHaContext ha, ILogger<MpptNightApp> logger, EntitiesManagerService entitiesManagerService)
        : base(ha, logger, entitiesManagerService)
    {
        Entity sunRisingEntity = ha.GetAllEntities().First(e => e.EntityId == "sensor.sun_next_rising");
        MqttEntity timeSleepEntity = EntitiesManagerService.Entities.TimeSleep;
        
        EntitiesManagerService.Entities.MpptNight.TurnedOn(logger, true)
            .Where(_ => timeSleepEntity.IsOff(logger))
            .SubscribeAsync(async _ =>
        {
            DateTime sunRisingDateTime = DateTime.Parse(sunRisingEntity.State!);
            bool useSun = EntitiesManagerService.Entities.MpptNightUseSun.IsOn(logger); 
            
            TimeSpan timeSleep = TimeSpan.FromHours(10);
            TimeSpan durationBeforeSunRinsing = DateTime.UtcNow - sunRisingDateTime;
            
            Logger.LogDebug("Sun rising is in {duration} ==> {sunRisingDateTime}", durationBeforeSunRinsing, sunRisingDateTime);

            if (useSun)
            {
                timeSleep = durationBeforeSunRinsing;
            }
            
            if (EntitiesManagerService.Entities.MpptNightTurnOn.IsOn(logger))
            {
                timeSleep = TimeSpan.FromMinutes(EntitiesManagerService.Entities.MpptNightTimeOff.State.ToInt(60));

                if (useSun && timeSleep > durationBeforeSunRinsing)
                {
                    logger.LogInformation("Sleep too long and we will miss sunrise so sleep to sunrise in {sunDuration} instead of {duration}", durationBeforeSunRinsing, timeSleep);

                    timeSleep = durationBeforeSunRinsing;
                }
            }
            
            entitiesManagerService.Update(timeSleepEntity, timeSleep.TotalMinutes);
            await entitiesManagerService.UpdateEntities();
        });
    }
}