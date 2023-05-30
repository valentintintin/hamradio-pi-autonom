using NetDaemon.Extensions.MqttEntityManager;
using NetDaemon.HassModel;

namespace Monitor.WorkServices;

public class EntitiesManagerService : AService
{
    private readonly IMqttEntityManager _mqttEntityManager;
    private readonly IHaContext _haContext;

    public EntitiesManagerService(ILogger<EntitiesManagerService> logger, IMqttEntityManager mqttEntityManager,
        IHaContext haContext) : base(logger)
    {
        _mqttEntityManager = mqttEntityManager;
        _haContext = haContext;
    }

    public async Task<BinarySensorEntity> Boolean(string id, string name, BinarySensorDeviceClass? deviceClass = null, bool? value = null)
    {
        BinarySensorEntity entity = new(_haContext, $"binary_sensor.{id}");
        
        await _mqttEntityManager.CreateAsync(entity.EntityId, new EntityCreationOptions
        {
            Name = name,
            DeviceClass = deviceClass?.ToString().ToLower()
        });

        if (value.HasValue)
        {
            await _mqttEntityManager.SetStateAsync(entity.EntityId, value == true ? "true" : "false");
        }
        
        return entity;
    }

    public async Task<SensorEntity> Value(string id, string name, SensorDeviceClass? deviceClass = null, string? value = null, string? unit = null)
    {
        SensorEntity entity = new(_haContext, $"binary_sensor.{id}");
        
        await _mqttEntityManager.CreateAsync(entity.EntityId, new EntityCreationOptions
        {
            Name = name,
            DeviceClass = deviceClass?.ToString().ToLower()
        });

        if (!string.IsNullOrWhiteSpace(value))
        {
            await _mqttEntityManager.SetStateAsync(entity.EntityId, value);
        }
        
        return entity;
    }
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

