using Monitor.Context;
using Monitor.Context.Entities;
using Monitor.Extensions;
using Monitor.Models;
using Monitor.Models.SerialMessages;
using NetDaemon.HassModel;
using NetDaemon.HassModel.Entities;

namespace Monitor.WorkServices;

public class MonitorService : AService
{
    public static readonly MonitorState State = new();

    private readonly DataContext _context;
    private readonly SystemService _systemService;
    private readonly EntitiesManagerService _entitiesManagerService;
    
    public MonitorService(ILogger<MonitorService> logger, DataContext context,
        SystemService systemService, EntitiesManagerService entitiesManagerService) : base(logger)
    {
        _context = context;
        _systemService = systemService;
        _entitiesManagerService = entitiesManagerService; 
    }

    public async Task UpdateStateFromMessage(Message message)
    {
        State.LastMessagesReceived.Add(message);
        
        switch (message)
        {
            case McuSystemData systemData:
                State.McuSystem = systemData;
                
                _entitiesManagerService.Update(EntitiesManagerService.Entities.McuStatus, systemData.State);
                _entitiesManagerService.Update(EntitiesManagerService.Entities.SystemBoxOpened, systemData.BoxOpened);
                break;
            case WeatherData weatherData:
                State.Weather = weatherData;
                
                _entitiesManagerService.Update(EntitiesManagerService.Entities.WeatherTemperature, weatherData.Temperature);
                _entitiesManagerService.Update(EntitiesManagerService.Entities.WeatherHumidity, weatherData.Humidity);

                _context.Add(new Weather
                {
                    Temperature = weatherData.Temperature,
                    Humidity = weatherData.Humidity
                });
                _context.SaveChanges();
                break;
            case MpptData mpptData:
                State.Mppt = mpptData;
                
                _entitiesManagerService.Update(EntitiesManagerService.Entities.MpptBatteryVoltage, mpptData.BatteryVoltage);
                _entitiesManagerService.Update(EntitiesManagerService.Entities.MpptBatteryCurrent, mpptData.BatteryCurrent);
                _entitiesManagerService.Update(EntitiesManagerService.Entities.MpptSolarVoltage, mpptData.SolarVoltage);
                _entitiesManagerService.Update(EntitiesManagerService.Entities.MpptSolarCurrent, mpptData.SolarCurrent);
                _entitiesManagerService.Update(EntitiesManagerService.Entities.MpptChargeCurrent, mpptData.CurrentCharge);
                _entitiesManagerService.Update(EntitiesManagerService.Entities.MpptStatus, mpptData.StatusString);
                _entitiesManagerService.Update(EntitiesManagerService.Entities.MpptNight, mpptData.Night);
                _entitiesManagerService.Update(EntitiesManagerService.Entities.MpptAlert, mpptData.Alert);
                _entitiesManagerService.Update(EntitiesManagerService.Entities.MpptPower, mpptData.PowerEnabled);
                _entitiesManagerService.Update(EntitiesManagerService.Entities.MpptWatchdog, mpptData.WatchdogEnabled);
                _entitiesManagerService.Update(EntitiesManagerService.Entities.MpptWatchdogCounter, mpptData.WatchdogCounter);
                _entitiesManagerService.Update(EntitiesManagerService.Entities.MpptWatchdogPowerOffTime, mpptData.WatchdogPowerOffTime);
                _entitiesManagerService.Update(EntitiesManagerService.Entities.MpptPowerOffVoltage, mpptData.PowerOffVoltage);
                _entitiesManagerService.Update(EntitiesManagerService.Entities.MpptPowerOnVoltage, mpptData.PowerOnVoltage);

                _context.Add(new Mppt
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
                _context.SaveChanges();
                break;
            case TimeData timeData:
                State.Time = timeData;
                
                _entitiesManagerService.Update(EntitiesManagerService.Entities.McuUptime, timeData.Uptime);
                
                _systemService.SetTime(State.Time.DateTime.DateTime);
                break;
            case GpioData gpioData:
                State.Gpio = gpioData;
                
                _entitiesManagerService.Update(EntitiesManagerService.Entities.GpioWifi, gpioData.Wifi);
                _entitiesManagerService.Update(EntitiesManagerService.Entities.GpioNpr, gpioData.Npr);
                _entitiesManagerService.Update(EntitiesManagerService.Entities.GpioBoxLdr, gpioData.Ldr);
                break;
            case LoraData loraData:
                if (loraData.IsTx)
                {
                    State.Lora.LastTx.Add(loraData.Payload);
                
                    _entitiesManagerService.Update(EntitiesManagerService.Entities.LoraTxPayload, loraData.Payload);
                }
                else
                {
                    State.Lora.LastRx.Add(loraData.Payload);
                
                    _entitiesManagerService.Update(EntitiesManagerService.Entities.LoraRxPayload, loraData.Payload);
                }
                break;
        }

        await _entitiesManagerService.UpdateEntities();
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
}