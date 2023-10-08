using Monitor.Extensions;
using Monitor.Models;
using Monitor.Services;
using SunCalcNet;
using SunCalcNet.Model;

namespace Monitor.Workers;

public class PowerNightApp : AEnabledWorker
{
    public static readonly MqttEntity<bool> TurnOn = new("night/turn_on", true);
    public static readonly MqttEntity<int> LimitVoltage = new("night/limit_voltage", true, 11600);
    public static readonly MqttEntity<TimeSpan> TimeOff = new("night/time_off", true, TimeSpan.FromMinutes(60));
    public static readonly MqttEntity<bool> UseSun = new("night/use_sun", true);
    
    private readonly (double latitude, double longitude, int altitude) _position;
    private readonly SystemService _systemService;

    public PowerNightApp(ILogger<PowerNightApp> logger, IServiceProvider serviceProvider, IConfiguration configuration)
        : base(logger, serviceProvider)
    {
        IConfigurationSection configurationSection = configuration.GetSection("Position");

        _position = (
            configurationSection.GetValueOrThrow<double>("Latitude"),
            configurationSection.GetValueOrThrow<double>("Longitude"),
            configurationSection.GetValueOrThrow<int>("Altitude")
        );

        EntitiesManagerService.Add(TurnOn);
        EntitiesManagerService.Add(TimeOff);
        EntitiesManagerService.Add(UseSun);
        EntitiesManagerService.Add(LimitVoltage);

        _systemService = Services.GetRequiredService<SystemService>();
    }

    private void Do()
    {
        if (_systemService.IsShutdownAsked())
        {
            Logger.LogDebug("Night but time sleep already set");
            
            return;
        }

        TimeSpan timeSleep = TimeOff.Value;
        bool turnOn = TurnOn.IsTrue();
        int batteryVoltage = EntitiesManagerService.Entities.BatteryVoltage.Value;
        int limitVoltage = LimitVoltage.Value;
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
        
        if (UseSun.IsTrue())
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

    protected override Task Start()
    {
        AddDisposable(EntitiesManagerService.Entities.SolarIsDay.IsFalseAsync()
            .Subscribe(_ =>
            {
                Do();
            })
        );
        
        return Task.CompletedTask;
    }
}