using Microsoft.EntityFrameworkCore;
using Monitor.Context;
using Monitor.Context.Entities;
using Monitor.Models;
using Monitor.Models.SerialMessages;

namespace Monitor.Services;

public class MonitorService : AService
{
    public static readonly MonitorState State = new();

    private readonly IDbContextFactory<DataContext> _contextFactory;
    private readonly SystemService _systemService;
    
    public MonitorService(ILogger<MonitorService> logger, IDbContextFactory<DataContext> contextFactory, SystemService systemService) : base(logger)
    {
        _contextFactory = contextFactory;
        _systemService = systemService;
    }

    public async Task UpdateStateFromMessage(Message message)
    {
        DataContext context = await _contextFactory.CreateDbContextAsync();

        State.LastMessagesReceived.Add(message);
        
        switch (message)
        {
            case McuSystemData systemData:
                Logger.LogTrace("New system data received");
                
                State.McuSystem = systemData;
                
                EntitiesManagerService.Entities.McuStatus.SetValue(systemData.State);
                EntitiesManagerService.Entities.StatusBoxOpened.SetValue(systemData.BoxOpened);
                break;
            case WeatherData weatherData:
                Logger.LogTrace("New weather data received");

                State.Weather = weatherData;
                
                EntitiesManagerService.Entities.WeatherTemperature.SetValue(weatherData.Temperature);
                EntitiesManagerService.Entities.WeatherHumidity.SetValue(weatherData.Humidity);

                context.Add(new Weather
                {
                    Temperature = weatherData.Temperature,
                    Humidity = weatherData.Humidity
                });
                await context.SaveChangesAsync();
                break;
            case MpptData mpptData:
                Logger.LogTrace("New MPPT data received");

                State.Mppt = mpptData;
                
                EntitiesManagerService.Entities.BatteryVoltage.SetValue(mpptData.BatteryVoltage);
                EntitiesManagerService.Entities.BatteryCurrent.SetValue(mpptData.BatteryCurrent);
                EntitiesManagerService.Entities.SolarVoltage.SetValue(mpptData.SolarVoltage);
                EntitiesManagerService.Entities.SolarCurrent.SetValue(mpptData.SolarCurrent);
                EntitiesManagerService.Entities.MpptChargeCurrent.SetValue(mpptData.CurrentCharge);
                EntitiesManagerService.Entities.MpptStatus.SetValue(mpptData.StatusString);
                EntitiesManagerService.Entities.SolarIsDay.SetValue(!mpptData.Night);
                EntitiesManagerService.Entities.MpptAlertShutdown.SetValue(mpptData.Alert);
                EntitiesManagerService.Entities.MpptPowerEnabled.SetValue(mpptData.PowerEnabled);
                EntitiesManagerService.Entities.WatchdogEnabled.SetValue(mpptData.WatchdogEnabled);
                EntitiesManagerService.Entities.WatchdogCounter.SetValue(TimeSpan.FromSeconds(mpptData.WatchdogCounter));
                EntitiesManagerService.Entities.WatchdogPowerOffTime.SetValue(TimeSpan.FromSeconds(mpptData.WatchdogPowerOffTime));
                EntitiesManagerService.Entities.MpptPowerOffVoltage.SetValue(mpptData.PowerOffVoltage);
                EntitiesManagerService.Entities.MpptPowerOnVoltage.SetValue(mpptData.PowerOnVoltage);

                context.Add(new Mppt
                {
                    BatteryVoltage = mpptData.BatteryVoltage,
                    BatteryCurrent = mpptData.BatteryCurrent,
                    SolarVoltage = mpptData.SolarVoltage,
                    SolarCurrent = mpptData.SolarCurrent,
                    CurrentCharge = mpptData.CurrentCharge,
                    Status = mpptData.Status,
                    StatusString = mpptData.StatusString,
                    Night = mpptData.Night,
                    Alert = mpptData.Alert,
                    WatchdogEnabled = mpptData.WatchdogEnabled,
                    WatchdogPowerOffTime = mpptData.WatchdogPowerOffTime
                });
                await context.SaveChangesAsync();
                break;
            case TimeData timeData:
                Logger.LogTrace("New time data received");

                State.Time = timeData;
                
                EntitiesManagerService.Entities.McuUptime.SetValue(timeData.UptimeTimeSpan);
                
                _systemService.SetTime(State.Time.DateTime.DateTime);
                break;
            case GpioData gpioData:
                Logger.LogTrace("New GPIO data received");

                State.Gpio = gpioData;
                
                EntitiesManagerService.Entities.GpioWifi.SetValue(gpioData.Wifi);
                EntitiesManagerService.Entities.GpioNpr.SetValue(gpioData.Npr);
                EntitiesManagerService.Entities.GpioBoxLdr.SetValue(gpioData.Ldr);

                context.Add(new Context.Entities.System
                {
                    Npr = gpioData.Npr,
                    Wifi = gpioData.Wifi,
                    Uptime = (long)EntitiesManagerService.Entities.SystemUptime.Value.TotalSeconds,
                    BoxOpened = State.McuSystem.BoxOpened,
                    McuUptime = State.Time.Uptime
                });
                await context.SaveChangesAsync();
                break;
            case LoraData loraData:
                Logger.LogTrace("New LoRa data received");

                if (loraData.IsTx)
                {
                    State.Lora.LastTx.Add((loraData.Payload, DateTime.UtcNow));
                
                    EntitiesManagerService.Entities.LoraTxPayload.SetValue(loraData.Payload);
                }
                else
                {
                    State.Lora.LastRx.Add((loraData.Payload, DateTime.UtcNow));
                
                    EntitiesManagerService.Entities.LoraRxPayload.SetValue(loraData.Payload);
                }
                break;
        }
    }

    public void UpdateSystemState()
    {
        State.System = _systemService.GetInfo();

        if (State.System != null)
        {
            EntitiesManagerService.Entities.SystemUptime.SetValue(State.System.UptimeTimeSpan);
            EntitiesManagerService.Entities.SystemCpu.SetValue(State.System.Cpu);
            EntitiesManagerService.Entities.SystemRam.SetValue(State.System.Ram);
            EntitiesManagerService.Entities.SystemDisk.SetValue(State.System.Disk);
        }
    }
}