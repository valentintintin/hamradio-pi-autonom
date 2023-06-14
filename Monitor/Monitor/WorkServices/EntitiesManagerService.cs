using System.Reactive.Linq;
using Monitor.Models.HomeAssistant;
using NetDaemon.Extensions.MqttEntityManager;
using NetDaemon.HassModel;
using NetDaemon.HassModel.Entities;

namespace Monitor.WorkServices;

public class EntitiesManagerService : AService
{
    public static MqttEntities Entities { get; private set; } = null!;

    private readonly List<MqttEntity> _entitiesToUpdate = new();
    private readonly IMqttEntityManager _mqttEntityManager;
    private readonly IHaContext _haContext;
    private readonly SerialMessageService _serialMessageService;

    public EntitiesManagerService(ILogger<EntitiesManagerService> logger, IMqttEntityManager mqttEntityManager,
        IHaContext haContext, SerialMessageService serialMessageService) : base(logger)
    {
        _mqttEntityManager = mqttEntityManager;
        _haContext = haContext;
        _serialMessageService = serialMessageService;
    }

    public void Update<T>(MqttEntity entity, T value)
    {
        switch (value)
        {
            case bool valueBool:
                entity.SetState(valueBool);
                break;
            case int valueInt:
                entity.SetState(valueInt);
                break;
            case long valueLong:
                entity.SetState(valueLong);
                break;
            case float valueFloat:
                entity.SetState(valueFloat);
                break;
            case double valueDouble:
                entity.SetState(valueDouble);
                break;
            case string valueString:
                entity.SetState(valueString);
                break;
            case DateTime valueDateTime:
                entity.SetState(valueDateTime);
                break;
            case TimeSpan valueTimeSpan:
                entity.SetState(valueTimeSpan);
                break;
        }

        _entitiesToUpdate.Add(entity);
    }
    
    public async Task UpdateEntities()
    {
        if (_entitiesToUpdate.Any())
        {
            Logger.LogTrace("Update {count} entities to MQTT if there is change", _entitiesToUpdate.Count);

            foreach (MqttEntity entity in _entitiesToUpdate)
            {
                if (entity.State?.ToLower() != entity.NewState?.ToLower())
                {
                    Logger.LogTrace("Update {entityId} to {state}. Old was {oldState}", entity.EntityId,
                        entity.NewState, entity.State);

                    await _mqttEntityManager.SetStateAsync(entity.EntityId, entity.NewState ?? "");
                }

                entity.ClearState();
            }

            _entitiesToUpdate.Clear();
        }
    }

    public async Task Init()
    {
        Logger.LogInformation("Init entities to MQTT");

        Entities = new MqttEntities
        {
            GpioWifi = await Switch("gpio_wifi", "Wifi", true),
            GpioNpr = await Switch("gpio_npr", "NPR", true),
            WifiShouldTurnOn = await Switch("gpio_wifi_should_turn_on", "Wifi should turn on"),
            NprShouldTurnOn = await Switch("gpio_npr_should_turn_on", "Npr should turn on"),
            GpioBoxLdr = await Sensor("gpio_box_ldr", "Box LDR", SensorDeviceClass.ILLUMINANCE),
            SystemBoxOpened = await BinarySensor("system_box_opened", "Box opened", BinarySensorDeviceClass.LOCK),
            McuStatus = await Sensor("mcu_status", "MCU Status", SensorDeviceClass.ENUM),
            McuUptime = await Sensor("mcu_uptime", "MCU Uptime", SensorDeviceClass.DURATION),
            WeatherTemperature = await Sensor("weather_temperature", "Weather Temperature",
                SensorDeviceClass.TEMPERATURE, null, "°C"),
            WeatherHumidity = await Sensor("weather_humidity", "Weather Humidity", SensorDeviceClass.HUMIDITY, null,
                "%"),
            MpptBatteryVoltage = await Sensor("mppt_battery_voltage", "MPPT Battery Voltage",
                SensorDeviceClass.VOLTAGE, null, "mV"),
            MpptBatteryCurrent = await Sensor("mppt_battery_current", "MPPT Battery Current",
                SensorDeviceClass.CURRENT, null, "mA"),
            MpptSolarVoltage = await Sensor("mppt_solar_voltage", "MPPT Solar Voltage", SensorDeviceClass.VOLTAGE,
                null, "mV"),
            MpptSolarCurrent = await Sensor("mppt_solar_current", "MPPT Solar Current", SensorDeviceClass.CURRENT,
                null, "mA"),
            MpptChargeCurrent = await Sensor("mppt_charge_current", "MPPT Charge Current",
                SensorDeviceClass.CURRENT, null, "mA"),
            MpptStatus = await Sensor("mppt_status", "MPPT Status", SensorDeviceClass.ENUM),
            MpptDay = await BinarySensor("mppt_day", "MPPT Day", BinarySensorDeviceClass.LIGHT),
            MpptAlert = await BinarySensor("mppt_alert", "MPPT Alert", BinarySensorDeviceClass.PROBLEM),
            MpptPower = await BinarySensor("mppt_power", "MPPT Power", BinarySensorDeviceClass.POWER),
            MpptWatchdog = await BinarySensor("mppt_watchdog", "MPPT Watchdog", BinarySensorDeviceClass.RUNNING),
            MpptWatchdogCounter = await Sensor("mppt_watchdog_counter", "MPPT Watchdog Counter", null, null, "s"),
            MpptWatchdogPowerOffTime = await Sensor("mppt_watchdog_poweroff_time", "MPPT Watchdog PowerOff Time",
                null, null, "s"),
            MpptWatchdogSwitchOff = await Button("mppt_watchdog_switch_off", "MPPT Watchdog Switch Off",
                () => _serialMessageService.SetWatchdog(TimeSpan.Zero)),
            MpptPowerOffVoltage = await Number("mppt_poweroff_voltage", "MPPT Power Off Voltage",
                NumberDeviceClass.VOLTAGE, null, "mV", 11000, 13000),
            MpptPowerOnVoltage = await Number("mppt_poweron_voltage", "MPPT Power On Voltage",
                NumberDeviceClass.VOLTAGE, null, "mV", 12000, 13000),
            MpptLowBatteryVoltage = await Number("mppt_low_battery_voltage", "MPPT Low Battery Voltage",
                NumberDeviceClass.VOLTAGE, null, "mV", 11000, 13000),
            MpptLowBatteryTimeOff = await Number("mppt_low_battery_time_off", "MPPT Low Battery Time Off", null,
                null, "min", 5, 1000),
            MpptNightTurnOn = await Switch("mppt_night_turn_on", "MPPT Night Turn On"),
            MpptNightLimitVoltage = await Number("mppt_night_limit_voltage", "MPPT Night Limit Voltage",
                NumberDeviceClass.VOLTAGE, null, "mV", 11000, 13000),
            MpptNightTimeOff =
                await Number("mppt_night_time_off", "MPPT Night Time Off", null, null, "min", 5, 1000),
            MpptNightUseSun = await Switch("mppt_night_use_sun", "MPPT Night Use Sun"),
            LoraTxPayload = await Text("lora_tx_payload", "LoRa TX Payload"),
            LoraRxPayload = await Sensor("lora_rx_payload", "LoRa RX Payload"),
            TimeSleep = await Number("will_turn_off", "System will turn off", null, "0", "min", 0, 1000),
            Uptime = _haContext.Entity("sensor.uptime"),
            LastBoot = _haContext.Entity("sensor.last_boot"),
            SunRising = _haContext.Entity("sensor.sun_next_rising")
        };
    }

    private async Task<MqttEntity> Button(string id, string name, Action callback)
    {
        MqttEntity entity = new(_haContext, $"button.{id}");
        
        await _mqttEntityManager.CreateAsync(entity.EntityId, new EntityCreationOptions
        {
            Name = name,
        });
        
        (await _mqttEntityManager.PrepareCommandSubscriptionAsync(entity.EntityId))
            .Subscribe(_ =>
            {
                Logger.LogInformation("Action for entity {entityId} requested", entity.EntityId);
                callback();
            });
        
        return entity;
    }

    private async Task<MqttEntity> Select(string id, string name, List<string> options, string? value = null)
    {
        MqttEntity entity = new(_haContext, $"select.{id}");
        
        await _mqttEntityManager.CreateAsync(entity.EntityId, new EntityCreationOptions
        {
            Name = name,
        }, new
        {
            options
        });

        if (!string.IsNullOrWhiteSpace(value))
        {
            await _mqttEntityManager.SetStateAsync(entity.EntityId, value);
        }
        
        (await _mqttEntityManager.PrepareCommandSubscriptionAsync(entity.EntityId))
            .SubscribeAsync(async state =>
            {
                Logger.LogInformation("Change of entity {entityId} to {state}", entity.EntityId, state);
                await _mqttEntityManager.SetStateAsync(entity.EntityId, state);
            });
        
        return entity;
    }

    private async Task<MqttEntity> BinarySensor(string id, string name, BinarySensorDeviceClass? deviceClass = null, bool? value = null)
    {
        MqttEntity entity = new(_haContext, $"binary_sensor.{id}");
        
        await _mqttEntityManager.CreateAsync(entity.EntityId, new EntityCreationOptions
        {
            Name = name,
            DeviceClass = deviceClass?.ToString().ToLower()
        });

        if (value.HasValue)
        {
            await _mqttEntityManager.SetStateAsync(entity.EntityId, value == true ? "on" : "off");
        }
        
        return entity;
    }

    private async Task<MqttEntity> Switch(string id, string name, bool isOutlet = false, bool? value = null)
    {
        MqttEntity entity = new(_haContext, $"switch.{id}");
        
        await _mqttEntityManager.CreateAsync(entity.EntityId, new EntityCreationOptions
        {
            Name = name,
            DeviceClass = isOutlet ? "outlet" : "switch"
        });

        if (value.HasValue)
        {
            await _mqttEntityManager.SetStateAsync(entity.EntityId, value == true ? "on" : "off");
        }
        
        (await _mqttEntityManager.PrepareCommandSubscriptionAsync(entity.EntityId))
            .Distinct()
            .SubscribeAsync(async state =>
            {
                Logger.LogInformation("Change of entity {entityId} to {state}", entity.EntityId, state);
                await _mqttEntityManager.SetStateAsync(entity.EntityId, state);
            });
        
        return entity;
    }

    private async Task<MqttEntity> Number(string id, string name, NumberDeviceClass? deviceClass = null, string? value = null, string? unit = null, double min = 0, double max = 100)
    {
        MqttEntity entity = new(_haContext, $"number.{id}");
        
        await _mqttEntityManager.CreateAsync(entity.EntityId, new EntityCreationOptions
        {
            Name = name,
            DeviceClass = deviceClass?.ToString().ToLower()
        }, new
        {
            unit_of_measurement = unit,
            mode = "box",
            min,
            max
        });

        if (!string.IsNullOrWhiteSpace(value))
        {
            await _mqttEntityManager.SetStateAsync(entity.EntityId, value);
        }
        
        (await _mqttEntityManager.PrepareCommandSubscriptionAsync(entity.EntityId))
            .SubscribeAsync(async state =>
            {
                Logger.LogInformation("Change of entity {entityId} to {state}", entity.EntityId, state);
                await _mqttEntityManager.SetStateAsync(entity.EntityId, state);
            });
        
        return entity;
    }

    private async Task<MqttEntity> Sensor(string id, string name, SensorDeviceClass? deviceClass = null, string? value = null, string? unit = null)
    {
        MqttEntity entity = new(_haContext, $"sensor.{id}");
        
        await _mqttEntityManager.CreateAsync(entity.EntityId, new EntityCreationOptions
        {
            Name = name,
            DeviceClass = deviceClass?.ToString().ToLower()
        }, new
        {
            unit_of_measurement = unit
        });

        if (!string.IsNullOrWhiteSpace(value))
        {
            await _mqttEntityManager.SetStateAsync(entity.EntityId, value);
        }
        
        return entity;
    }

    private async Task<MqttEntity> Text(string id, string name, string? value = null)
    {
        MqttEntity entity = new(_haContext, $"text.{id}");
        
        await _mqttEntityManager.CreateAsync(entity.EntityId, new EntityCreationOptions
        {
            Name = name,
        });

        if (!string.IsNullOrWhiteSpace(value))
        {
            await _mqttEntityManager.SetStateAsync(entity.EntityId, value);
        }
        
        (await _mqttEntityManager.PrepareCommandSubscriptionAsync(entity.EntityId))
            .SubscribeAsync(async state =>
            {
                Logger.LogInformation("Change of entity {entityId} to {state}", entity.EntityId, state);
                await _mqttEntityManager.SetStateAsync(entity.EntityId, state);
            });
        
        return entity;
    }
}

public record MqttEntities
{
    public required MqttEntity GpioWifi { get; init; }
    public required MqttEntity GpioNpr { get; init; }
    public required MqttEntity GpioBoxLdr { get; init; }
    
    public required MqttEntity SystemBoxOpened { get; init; }
    
    public required MqttEntity McuStatus { get; init; }
    public required MqttEntity McuUptime { get; init; }
    
    public required MqttEntity WeatherTemperature { get; init; }
    public required MqttEntity WeatherHumidity { get; init; }
    
    public required MqttEntity MpptBatteryVoltage { get; init; }
    public required MqttEntity MpptBatteryCurrent { get; init; }
    public required MqttEntity MpptSolarVoltage { get; init; }
    public required MqttEntity MpptSolarCurrent { get; init; }
    public required MqttEntity MpptChargeCurrent { get; init; }
    public required MqttEntity MpptStatus { get; init; }
    public required MqttEntity MpptDay { get; init; }
    public required MqttEntity MpptAlert { get; init; }
    public required MqttEntity MpptPower { get; init; }
    public required MqttEntity MpptWatchdog { get; init; }
    public required MqttEntity MpptWatchdogCounter { get; init; }
    public required MqttEntity MpptWatchdogPowerOffTime { get; init; }
    public required MqttEntity MpptWatchdogSwitchOff { get; init; }
    public required MqttEntity MpptPowerOffVoltage { get; init; }
    public required MqttEntity MpptPowerOnVoltage { get; init; }
    
    public required MqttEntity MpptNightTurnOn { get; init; }
    public required MqttEntity MpptNightLimitVoltage { get; init; }
    public required MqttEntity MpptNightTimeOff { get; init; }
    public required MqttEntity MpptNightUseSun { get; init; }
    
    public required MqttEntity MpptLowBatteryVoltage { get; init; }
    public required MqttEntity MpptLowBatteryTimeOff { get; init; }
    
    public required MqttEntity LoraTxPayload { get; init; }
    public required MqttEntity LoraRxPayload { get; init; }
    
    public required MqttEntity TimeSleep { get; init; }
    
    public required MqttEntity WifiShouldTurnOn { get; init; }
    public required MqttEntity NprShouldTurnOn { get; init; }
    
    public Entity? Uptime { get; init; }
    public Entity? LastBoot { get; init; }

    public Entity? SunRising { get; init; }
}

public enum BinarySensorDeviceClass
{
    BATTERY,
    BATTERY_CHARGING,
    CO,
    COLD,
    CONNECTIVITY,
    DOOR,
    GARAGE_DOOR,
    GAS,
    HEAT,
    LIGHT,
    LOCK,
    MOISTURE,
    MOTION,
    MOVING,
    OCCUPANCY,
    OPENING,
    PLUG,
    POWER,
    PRESENCE,
    PROBLEM,
    RUNNING,
    SAFETY,
    SMOKE,
    SOUND,
    TAMPER,
    UPDATE,
    VIBRATION,
    WINDOW
}

public enum SensorDeviceClass
{
    APPARENT_POWER,
    AQI,
    ATMOSPHERIC_PRESSURE,
    BATTERY,
    CARBON_DIOXIDE,
    CARBON_MONOXIDE,
    CURRENT,
    DATA_RATE,
    DATA_SIZE,
    DATE,
    DISTANCE,
    DURATION,
    ENERGY,
    ENERGY_STORAGE,
    ENUM,
    FREQUENCY,
    GAS,
    HUMIDITY,
    ILLUMINANCE,
    IRRADIANCE,
    MOISTURE,
    MONETARY,
    NITROGEN_DIOXIDE,
    NITROGEN_MONOXIDE,
    NITROUS_OXIDE,
    OZONE,
    PM1,
    PM25,
    PM10,
    POWER,
    POWER_FACTOR,
    PRECIPITATION,
    PRECIPITATION_INTENSITY,
    PRESSURE,
    REACTIVE_POWER,
    SIGNAL_STRENGTH,
    SOUND_PRESSURE,
    SPEED,
    SULPHUR_DIOXIDE,
    TEMPERATURE,
    TIMESTAMP,
    VOLATILE_ORGANIC_COMPOUNDS,
    VOLATILE_ORGANIC_COMPOUNDS_PARTS,
    VOLTAGE,
    VOLUME,
    VOLUME_STORAGE,
    WATER,
    WEIGHT,
    WIND_SPEED
}

public enum NumberDeviceClass
{
    APPARANT_POWER, // VA
    AQI, // None
    ATMOSPHERIC_PRESSURE, // cbar, bar, hPa, inHg, kPa, mbar, Pa, psi
    BATTERY, // %
    CARBON_DIOXIDE, // ppm
    CARBON_MONOXIDE, // ppm
    CURRENT, // A, mA
    DATA_RATE, // bit/s, kbit/s, Mbit/s, Gbit/s, B/s, kB/s, MB/s, GB/s, KiB/s, MiB/s, GiB/s
    DATA_SIZE, // bit, kbit, Mbit, Gbit, B, kB, MB, GB, TB, PB, EB, ZB, YB, KiB, MiB, GiB, TiB, PiB, EiB, ZiB, YiB
    DISTANCE, // km, m, cm, mm, mi, yd, in
    ENERGY, // Wh, kWh, MWh, MJ, GJ
    ENERGY_STORAGE, // Wh, kWh, MWh, MJ, GJ
    FREQUENCY, // Hz, kHz, MHz, GHz
    GAS, // m³, ft³, CCF
    HUMIDITY, // %
    ILLUMINANCE, // lx
    IRRADIANCE, // W/m², BTU/(h⋅ft²)
    MOISTURE, // %
    MONETARY, // ISO 4217
    NITROGEN_DIOXIDE, // µg/m³
    NITROGEN_MONOXIDE, // µg/m³
    NITROUS_OXIDE, // µg/m³
    OZONE, // µg/m³
    PM1, // µg/m³
    PM25, // µg/m³
    PM10, // µg/m³
    POWER, // W, kW
    POWER_FACTOR, // %, None
    PRECIPITATION, // cm, in, mm
    PRECIPITATION_INTENSITY, // in/d, in/h, mm/d, mm/h
    PRESSURE, // cbar, bar, hPa, inHg, kPa, mbar, Pa, psi
    REACTIVE_POWER, // var
    SIGNAL_STRENGTH, // dB, dBm
    SOUND_PRESSURE, // dB, dBA
    SPEED, // ft/s, in/d, in/h, km/h, kn, m/s, mph, mm/d
    SULPHUR_DIOXIDE, // µg/m³
    TEMPERATURE, // °C, °F, K
    VOLATILE_ORGANIC_COMPOUNDS, // µg/m³
    VOLTAGE, // V, mV
    VOLUME, // L, mL, gal, fl. oz., m³, ft³, CCF
    VOLUME_STORAGE, // L, mL, gal, fl. oz., m³, ft³, CCF
    WATER, // L, gal, m³, ft³, CCF
    WEIGHT, // kg, g, mg, µg, oz, lb, st
    WIND_SPEED // ft/s, in/d, in/h, km/h, kn, m/s, mph, mm/d
}