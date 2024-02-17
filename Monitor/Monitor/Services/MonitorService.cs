using AprsSharp.AprsParser;
using Microsoft.EntityFrameworkCore;
using Monitor.Context;
using Monitor.Context.Entities;
using Monitor.Models;
using Monitor.Models.SerialMessages;

namespace Monitor.Services;

public class MonitorService(
    ILogger<MonitorService> logger,
    IDbContextFactory<DataContext> contextFactory,
    SystemService systemService)
    : AService(logger)
{
    public static readonly MonitorState State = new();

    public async Task UpdateStateFromMessage(Message message)
    {
        var context = await contextFactory.CreateDbContextAsync();

        State.LastMessagesReceived.Add(message);
        
        switch (message)
        {
            case McuSystemData systemData:
                Logger.LogDebug("New system data received : {message}", systemData);
                
                State.McuSystem = systemData;
                
                EntitiesManagerService.Entities.McuStatus.SetValue(systemData.State);
                EntitiesManagerService.Entities.StatusBoxOpened.SetValue(systemData.BoxOpened);
                EntitiesManagerService.Entities.FeatureWatchdogSafetyEnabled.SetValue(systemData.WatchdogSafetyEnabled);
                EntitiesManagerService.Entities.FeatureAprsDigipeaterEnabled.SetValue(systemData.AprsDigipeaterEnabled);
                EntitiesManagerService.Entities.FeatureAprsTelemetryEnabled.SetValue(systemData.AprsTelemetryEnabled);
                EntitiesManagerService.Entities.FeatureAprsPositionEnabled.SetValue(systemData.AprsPositionEnabled);
                EntitiesManagerService.Entities.FeatureSleepEnabled.SetValue(systemData.Sleep);
                EntitiesManagerService.Entities.TemperatureRtc.SetValue(systemData.TemperatureRtc);
                break;
            case WeatherData weatherData:
                Logger.LogDebug("New weather data received : {message}", weatherData);

                State.Weather = weatherData;
                
                EntitiesManagerService.Entities.WeatherTemperature.SetValue(weatherData.Temperature);
                EntitiesManagerService.Entities.WeatherHumidity.SetValue(weatherData.Humidity);
                EntitiesManagerService.Entities.WeatherPressure.SetValue(weatherData.Pressure);

                context.Add(new Weather
                {
                    Temperature = weatherData.Temperature,
                    Humidity = weatherData.Humidity,
                    Pressure = weatherData.Pressure,
                });
                await context.SaveChangesAsync();
                break;
            case MpptData mpptData:
                Logger.LogDebug("New MPPT data received : {message}", mpptData);

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
                EntitiesManagerService.Entities.MpptTemperature.SetValue(mpptData.Temperature);

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
                    WatchdogPowerOffTime = mpptData.WatchdogPowerOffTime,
                    Temperature = mpptData.Temperature
                });
                await context.SaveChangesAsync();
                break;
            case TimeData timeData:
                Logger.LogDebug("New time data received : {message}", timeData);

                State.Time = timeData;
                
                EntitiesManagerService.Entities.McuUptime.SetValue(timeData.UptimeTimeSpan);
                
                systemService.SetTime(State.Time.DateTime.DateTime);
                break;
            case GpioData gpioData:
                Logger.LogDebug("New GPIO data received : {message}", gpioData);

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
                    McuUptime = State.Time.Uptime,
                    TemperatureRtc = State.McuSystem.TemperatureRtc
                });
                await context.SaveChangesAsync();
                break;
            case LoraData loraData:
                Logger.LogDebug("New LoRa data received : {message}", loraData);
                
                if (loraData.IsTx)
                {
                    State.Lora.LastTx.Add(loraData);
                
                    EntitiesManagerService.Entities.LoraTxPayload.SetValue(loraData.Payload);
                }
                else
                {
                    State.Lora.LastRx.Add(loraData);
                
                    EntitiesManagerService.Entities.LoraRxPayload.SetValue(loraData.Payload);
                }

                string? sender = null;
                
                try
                {
                    Packet packet = new(loraData.Payload);
                    sender = packet.Sender;
                }
                catch (Exception e)
                {
                    Logger.LogWarning(e, "LoRa APRS frame received not decodable : {payload}", loraData.Payload);
                }

                context.Add(new LoRa
                {
                    Sender = sender,
                    Frame = loraData.Payload,
                    IsTx = loraData.IsTx
                });
                await context.SaveChangesAsync();
                break;
        }
    }

    public void UpdateSystemState(SystemState? systemState)
    {
        Logger.LogInformation("New system info received");
        
        State.System = systemState;

        if (State.System != null)
        {
            EntitiesManagerService.Entities.SystemUptime.SetValue(State.System.UptimeTimeSpan);
            EntitiesManagerService.Entities.SystemCpu.SetValue(State.System.Cpu);
            EntitiesManagerService.Entities.SystemRam.SetValue(State.System.Ram);
            EntitiesManagerService.Entities.SystemDisk.SetValue(State.System.Disk);
        }
    }
}