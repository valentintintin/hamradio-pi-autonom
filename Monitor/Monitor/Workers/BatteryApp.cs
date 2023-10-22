using System.Reactive.Linq;
using Monitor.Extensions;
using Monitor.Models;
using Monitor.Services;
using SunCalcNet;
using SunCalcNet.Model;

namespace Monitor.Workers;

public class BatteryApp : AWorker
{
    public static readonly StringConfigEntity<bool> NightEnabled = new("night/enabled", true);
    public static readonly StringConfigEntity<bool> NightTurnOn = new("night/turn_on", true);
    public static readonly StringConfigEntity<int> NightLimitVoltage = new("night/limit_voltage", true, 11600);
    public static readonly StringConfigEntity<TimeSpan> NightTimeOff = new("night/time_off", true, TimeSpan.FromMinutes(60));
    public static readonly StringConfigEntity<bool> NightUseSun = new("night/use_sun", true);
    
    public static readonly StringConfigEntity<bool> LowBatteryEnabled = new("low_battery/enabled", true);
    public static readonly StringConfigEntity<int> LowBatteryVoltage = new("low_battery/voltage", true, 11600);
    public static readonly StringConfigEntity<TimeSpan> LowBatteryTimeOff = new("low_battery/time_off", true, TimeSpan.FromMinutes(30));

    private readonly (double latitude, double longitude, int altitude) _position;
    private readonly SystemService _systemService;
    
    public BatteryApp(ILogger<BatteryApp> logger, IServiceProvider serviceProvider, IConfiguration configuration) 
        : base(logger, serviceProvider)
    {
        EntitiesManagerService.Add(LowBatteryEnabled);
        EntitiesManagerService.Add(LowBatteryVoltage);
        EntitiesManagerService.Add(LowBatteryTimeOff);
        
        IConfigurationSection configurationSection = configuration.GetSection("Position");

        _position = (
            configurationSection.GetValueOrThrow<double>("Latitude"),
            configurationSection.GetValueOrThrow<double>("Longitude"),
            configurationSection.GetValueOrThrow<int>("Altitude")
        );

        EntitiesManagerService.Add(NightEnabled);
        EntitiesManagerService.Add(NightTurnOn);
        EntitiesManagerService.Add(NightTimeOff);
        EntitiesManagerService.Add(NightUseSun);
        EntitiesManagerService.Add(NightLimitVoltage);
        
        _systemService = Services.GetRequiredService<SystemService>();
    }

    protected override Task Start()
    {
        AddDisposable(EntitiesManagerService.Entities.BatteryVoltage.ValueChanges()
            .Select(v => v.value)
            .Buffer(TimeSpan.FromMinutes(5))
            .Select(s => s.Any() ? s.Average() : int.MaxValue)
            .Where(_ => LowBatteryEnabled.IsTrue())
            .Where(_ => !NightEnabled.Value || EntitiesManagerService.Entities.SolarIsDay.IsTrue())
            .Do(s => Logger.LogDebug("Average Battery Voltage : {averageVoltage}", s))
            .Where(s => s < LowBatteryVoltage.Value)
            .Subscribe(s =>
            {
                Logger.LogWarning("Battery is too low so sleep. {batteryVoltage} < {lowVoltage}", s, LowBatteryVoltage.Value);

                _systemService.AskForShutdown(LowBatteryTimeOff.Value);
            }));
        
        AddDisposable(EntitiesManagerService.Entities.SolarIsDay.IsFalseAsync()
            .Where(_ => NightEnabled.IsTrue())
            .Subscribe(_ =>
            {
                DoNight();
            })
        );
        
        return Task.CompletedTask;
    }
    
    private void DoNight()
    {
        if (_systemService.IsShutdownAsked())
        {
            Logger.LogDebug("Night but time sleep already set");
            
            return;
        }

        TimeSpan timeSleep = NightTimeOff.Value;
        bool turnOn = NightTurnOn.IsTrue();
        int batteryVoltage = EntitiesManagerService.Entities.BatteryVoltage.Value;
        int limitVoltage = NightLimitVoltage.Value;
        DateTime sunRisingDateTime = SunCalc.GetSunPhases(DateTime.UtcNow, _position.latitude, _position.longitude, _position.altitude).First(e => e.Name.Value == SunPhaseName.Sunrise.Value).PhaseTime;

        if (sunRisingDateTime < DateTime.UtcNow)
        {
            sunRisingDateTime = sunRisingDateTime.AddDays(1);
        }
        
        if (batteryVoltage <= limitVoltage)
        {
            Logger.LogInformation("Night detected and Battery Voltage is {batteryVoltage} so below {limitVoltage}", batteryVoltage, limitVoltage);
            
            turnOn = false;
        }
        
        if (NightUseSun.IsTrue())
        {
            TimeSpan durationBeforeSunRinsing = sunRisingDateTime - DateTime.UtcNow;
            Logger.LogDebug("Sun rising is in {duration} ==> {sunRisingDateTime}", durationBeforeSunRinsing,
                sunRisingDateTime);
            
            if (DateTime.UtcNow < sunRisingDateTime)
            {
                if (turnOn)
                {
                    if (timeSleep > durationBeforeSunRinsing)
                    {
                        Logger.LogInformation(
                            "Sleep too long and we will miss sunrise so sleep to sunrise instead of {duration}",
                            timeSleep);

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
                Logger.LogDebug("We can't use sun rising because of false value");
            }
        }
        else
        {
            Logger.LogTrace("We do not use sun");
            
            if (!turnOn)
            {
                Logger.LogTrace("We do not turn on during night");
                
                timeSleep = TimeSpan.FromHours(10);
            }
        }

        _systemService.AskForShutdown(timeSleep);
    }
}