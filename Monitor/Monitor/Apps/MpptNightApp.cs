using System.Globalization;
using System.Reactive.Linq;
using Monitor.Extensions;
using Monitor.Models.HomeAssistant;
using Monitor.WorkServices;
using NetDaemon.AppModel;
using NetDaemon.HassModel;
using NetDaemon.HassModel.Entities;

namespace Monitor.Apps;

[NetDaemonApp(Id = "mppt_night_app")]
public class MpptNightApp : AApp, IAsyncInitializable
{
    private readonly EntitiesManagerService _entitiesManagerService;
    private readonly Entity _dayEntity;
    
    public MpptNightApp(IHaContext ha, ILogger<MpptNightApp> logger, EntitiesManagerService entitiesManagerService)
        : base(ha, logger, entitiesManagerService)
    {
        _entitiesManagerService = entitiesManagerService;
        _dayEntity = EntitiesManagerService.Entities.MpptDay;
        
        _dayEntity.TurnedOff(Logger)
            .SubscribeAsync(async _ =>
            {
                await Do();
            });
    }

    public async Task InitializeAsync(CancellationToken cancellationToken)
    {
        if (_dayEntity.IsOff(Logger))
        {
            await Do();
        }
    }

    private async Task Do()
    {
        MqttEntity entitiesTimeSleep = EntitiesManagerService.Entities.TimeSleep;
        
        if (!entitiesTimeSleep.IsOff(Logger))
        {
            Logger.LogDebug("Night but time sleep already set");
            
            return;
        }

        TimeSpan timeSleep = TimeSpan.FromMinutes(EntitiesManagerService.Entities.MpptNightTimeOff.State.ToInt(60));
        bool turnOn = EntitiesManagerService.Entities.MpptNightTurnOn.IsOn(Logger);
        bool useSun = EntitiesManagerService.Entities.MpptNightUseSun.IsOn(Logger);
        int batteryVoltage = EntitiesManagerService.Entities.MpptBatteryVoltage.State.ToInt();
        int limitVoltage = EntitiesManagerService.Entities.MpptNightLimitVoltage.State.ToInt();
        
        if (batteryVoltage <= limitVoltage)
        {
            Logger.LogInformation("Night detected and Battery Voltage is {batteryVoltage} so below {limitVoltage}", batteryVoltage, limitVoltage);
            
            turnOn = false;
        }
        
        if (useSun && DateTime.TryParse(EntitiesManagerService.Entities.SunRising?.State, CultureInfo.CurrentCulture, DateTimeStyles.AdjustToUniversal, out DateTime sunRisingDateTime))
        {
            TimeSpan durationBeforeSunRinsing = sunRisingDateTime - DateTime.UtcNow;
            Logger.LogDebug("Sun rising is in {duration} ==> {sunRisingDateTime}", durationBeforeSunRinsing, sunRisingDateTime);
            
            if (turnOn)
            {
                if (timeSleep > durationBeforeSunRinsing)
                {
                    Logger.LogInformation("Sleep too long and we will miss sunrise so sleep to sunrise instead of {duration}", timeSleep);

                    timeSleep = durationBeforeSunRinsing;
                }
            }
            else
            {
                Logger.LogDebug("We do not turn on during night so sleep to sun rising");
                
                timeSleep = durationBeforeSunRinsing;
            }
        }
        else
        {
            Logger.LogDebug("We do not use sun");
            
            if (!turnOn)
            {
                Logger.LogDebug("We do not turn on during night");
                
                timeSleep = TimeSpan.FromHours(10);
            }
        }

        _entitiesManagerService.Update(entitiesTimeSleep, Math.Round(timeSleep.TotalMinutes));
        await _entitiesManagerService.UpdateEntities();
    }
}