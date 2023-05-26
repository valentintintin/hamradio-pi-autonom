using Monitor.Context;
using Monitor.Context.Entities;
using Monitor.Extensions;
using Monitor.Models;
using Monitor.Models.SerialMessages;
using SunCalcNet;
using SunCalcNet.Model;

namespace Monitor.Services;

public class MonitorService : AService
{
    public static readonly MonitorState State = new();

    private readonly DataContext _context;
    private readonly IConfiguration _configuration;
    private readonly SystemService _systemService;
    private readonly SerialMessageService _serialMessageService;

    private readonly double _latitude;
    private readonly double _longitude;
    
    public MonitorService(ILogger<MonitorService> logger, DataContext context,
        IConfiguration configuration, SystemService systemService, SerialMessageService serialMessageService) : base(logger)
    {
        _context = context;
        _configuration = configuration;
        _systemService = systemService;
        _serialMessageService = serialMessageService;

        IConfigurationSection positionConfiguration = _configuration.GetSection("Position");
        _latitude = positionConfiguration.GetValueOrThrow<double>("Latitude");
        _longitude = positionConfiguration.GetValueOrThrow<double>("Longitude");
    }

    public void UpdateStateFromMessage(Message message)
    {
        State.LastMessagesReceived.Add(message);
        
        switch (message)
        {
            case McuSystemData systemData:
                State.McuSystem = systemData;
                break;
            case WeatherData weatherData:
                State.Weather = weatherData;

                _context.Add(new Weather
                {
                    Temperature = weatherData.Temperature,
                    Humidity = weatherData.Humidity
                });
                _context.SaveChanges();
                break;
            case MpptData mpptData:
                State.Mppt = mpptData;

                _context.Add(new Mppt
                {
                    BatteryVoltage = mpptData.BatteryVoltage,
                    BatteryCurrent = mpptData.BatteryCurrent,
                    SolarVoltage = mpptData.SolarVoltage,
                    SolarCurrent = mpptData.SolarCurrent,
                    CurrentCharge = mpptData.CurrentCharge,
                    Status = mpptData.Status,
                    Night = mpptData.Night,
                    Alert = mpptData.Alert,
                    WatchdogEnabled = mpptData.WatchdogEnabled,
                    WatchdogPowerOffTime = mpptData.WatchdogPowerOffTime
                });
                _context.SaveChanges();
                break;
            case TimeData timeData:
                State.Time = timeData;
                
                _systemService.SetTime(State.Time.DateTime.DateTime);
                break;
            case GpioData gpioData:
                if (gpioData.Name.ToUpper() == "WIFI")
                {
                    State.Gpio.Wifi = gpioData.Enabled;
                }
                else if (gpioData.Name.ToUpper() == "NPR")
                {
                    State.Gpio.Npr = gpioData.Enabled;
                }
                break;
            case LoraData loraData:
                if (loraData.IsTx)
                {
                    State.Lora.LastTx.Add(loraData.Payload);
                }
                else
                {
                    State.Lora.LastRx.Add(loraData.Payload);
                }
                break;
        }
    }

    public void AddLog(string log)
    {
        State.LastLogReceived.Add(log);
    }

    public void UpdateSystemInfo(SystemState state)
    {
        State.System = state;

        _context.Add(new Context.Entities.System
        {
            Npr = State.Gpio.Npr,
            Wifi = State.Gpio.Wifi,
            BoxOpened = State.McuSystem.BoxOpened,
            Uptime = State.System.Uptime,
            McuUptime = State.Time.Uptime
        });
        _context.SaveChanges();
    }

    public void ComputeBattery()
    {
        MpptData mppt = State.Mppt;
        IConfigurationSection configurationSection = _configuration.GetSection("Mppt");
        IConfigurationSection lowVoltageSection = configurationSection.GetSection("LowVoltage");
        
        int powerOnVoltage = configurationSection.GetValueOrThrow<int>("PowerOnVoltage");
        int powerOffVoltage = configurationSection.GetValueOrThrow<int>("PowerOffVoltage");

        if (mppt.PowerOnVoltage != powerOnVoltage || mppt.PowerOffVoltage != powerOffVoltage)
        {                    
            Logger.LogInformation("Set power off voltage to {powerOffVoltage} and power on voltage to {powerOnVoltage}", powerOffVoltage, powerOnVoltage);

            _serialMessageService.SetPowerOnOffVoltage(
                powerOnVoltage,
                powerOffVoltage
            );
        }
        
        if (mppt.Night)
        {
            Logger.LogInformation("Mppt is night, morning should be at {morning}", GetMorningDateTime());
            
            IConfigurationSection nightSection = configurationSection.GetSection("Night");

            if (nightSection.GetValueOrThrow<bool>("Enabled"))
            {
                if (mppt.BatteryVoltage < nightSection.GetValueOrThrow<int>("LimitVoltage"))
                {
                    Logger.LogInformation("Mppt battery too low {batteryVoltate}mV for night", mppt.BatteryVoltage);
                    
                    Sleep(TimeSpan.FromHours(nightSection.GetValueOrThrow<int>("TimeOffAllNightHours")));
                }
                else
                {
                    Logger.LogInformation("Mppt battery good {batteryVoltate}mV for night", mppt.BatteryVoltage);
                    
                    Sleep(TimeSpan.FromMinutes(nightSection.GetValueOrThrow<int>("TimeOffMinutes")));
                }
            }
            else
            {
                Logger.LogInformation("Mppt sleep for night");
                    
                Sleep(TimeSpan.FromHours(nightSection.GetValueOrThrow<int>("TimeOffAllNightHours")));
            }
        }
        else if (lowVoltageSection.GetValueOrThrow<bool>("Enabled")
                 && mppt.BatteryVoltage < lowVoltageSection.GetValueOrThrow<int>("Voltage"))
        {
            Logger.LogInformation("Mppt battery low voltage {batteryVoltage}", mppt.BatteryVoltage);
            
            Sleep(TimeSpan.FromMinutes(lowVoltageSection.GetValueOrThrow<int>("TimeOffMinutes")));
        }
    }

    private void Sleep(TimeSpan sleepDuration)
    {
        Logger.LogInformation("Sleep during {sleepTime} => {wakeUpTime}", sleepDuration, DateTime.UtcNow.Add(sleepDuration));
            
        _serialMessageService.SetWatchdog(sleepDuration);
    }

    public DateTime GetMorningDateTime()
    {
        DateTime sunToday = SunCalc.GetSunPhases(DateTime.UtcNow, _latitude, _longitude)
            .First(p => p.Name.Value == SunPhaseName.Dawn.Value).PhaseTime;

        DateTime sunTomorrow = SunCalc.GetSunPhases(DateTime.UtcNow.AddDays(1), _latitude, _longitude)
            .First(p => p.Name.Value == SunPhaseName.Dawn.Value).PhaseTime;
        
        return sunToday < DateTime.UtcNow ? sunTomorrow : sunToday;
    }
}