using System.Reactive.Concurrency;
using System.Reactive.Linq;
using System.Reflection;
using Microsoft.EntityFrameworkCore;
using Monitor.Context;
using Monitor.Context.Entities;
using Monitor.Extensions;
using Monitor.Models;
using MQTTnet;
using MQTTnet.Client;
using MQTTnet.Formatter;

namespace Monitor.Services;

public class EntitiesManagerService : AService, IAsyncDisposable
{
    public static MqttEntities Entities { get; } = new();

    private readonly string _topicBase;
    private readonly string _clientId;
    private readonly IMqttClient _mqttClient;
    private readonly IConfigurationSection _configurationSection;
    private readonly DataContext _context;
    private readonly List<IStringConfigEntity> _entities = new();
    private readonly IScheduler _scheduler;

    public EntitiesManagerService(ILogger<EntitiesManagerService> logger, 
        IConfiguration configuration, IServiceProvider serviceProvider,
        IDbContextFactory<DataContext> contextFactory) : base(logger)
    {
        _configurationSection = configuration.GetSection("Mqtt");
        _topicBase = _configurationSection.GetValueOrThrow<string>("TopicBase");
        _clientId = _configurationSection.GetValueOrThrow<string>("ClientId");
        _mqttClient = new MqttFactory().CreateMqttClient();
        _scheduler = serviceProvider.CreateScope().ServiceProvider.GetRequiredService<IScheduler>();
        _context = contextFactory.CreateDbContext();
        
        foreach (PropertyInfo property in typeof(MqttEntities).GetProperties())
        {
            IStringConfigEntity? entity = (IStringConfigEntity?)property.GetValue(Entities);
            Add(entity ?? throw new InvalidOperationException());
        }

        _mqttClient.ConnectedAsync += async _ =>
        {
            Logger.LogInformation("Connection successful to MQTT");
            
            await _mqttClient.SubscribeAsync(
                new MqttTopicFilterBuilder()
                    .WithTopic($"{_topicBase}/#")
                    .WithNoLocal()
                    .Build()
            );
        };

        _mqttClient.DisconnectedAsync += async args => 
        {
            logger.LogError(args.Exception, "MQTT disconnected");
            
            await Task.Delay(TimeSpan.FromSeconds(5));
            await ConnectMqtt();
        };
        
        // Observable.FromEvent<Func<MqttApplicationMessageReceivedEventArgs, Task>, MqttApplicationMessageReceivedEventArgs>(
        //         handler => args =>
        //         {
        //             handler(args);
        //             return Task.CompletedTask;
        //         },
        //         h => _mqttClient.ApplicationMessageReceivedAsync += h,
        //         h => _mqttClient.ApplicationMessageReceivedAsync -= h
        //     )
        //     .Select(m =>
        //     {
        //         string id = m.ApplicationMessage.Topic.Replace($"{_topicBase}/", "");
        //     
        //         IMqttEntity? entity = _entities.FirstOrDefault(entity => entity.Id == id);
        //         string payload = Encoding.UTF8.GetString(m.ApplicationMessage.PayloadSegment);
        //
        //         if (entity != null)
        //         {
        //             return new
        //             {
        //                 entity,
        //                 payload
        //             };
        //         }
        //
        //         Logger.LogError("From MQTT topic {topicId} not found with payload {payload}", id, payload);
        //         return null;
        //     })
        //     .Where(m => m != null)
        //     .Subscribe(m =>
        //     {
        //         try
        //         {
        //             if (m!.entity.SetFromMqttPayload(m.payload))
        //             {
        //                 Logger.LogInformation("Change from MQTT entity {entityId} to {payload}", m.entity.Id, m.payload);
        //             }
        //         }
        //         catch (Exception e)
        //         {
        //             Logger.LogError(e, "Change from MQTT error entity {entityId} to {payload}", m!.entity.Id, m.payload);
        //         }
        //     });
    }

    public async Task ConnectMqtt()
    {
        Logger.LogInformation("Run connection to MQTT");
        
        await _mqttClient.ConnectAsync(new MqttClientOptionsBuilder()
            .WithTcpServer(_configurationSection.GetValueOrThrow<string>("Host"), _configurationSection.GetValue("Port", 1883))
            .WithKeepAlivePeriod(TimeSpan.FromMinutes(1))
            .WithClientId(_clientId)
            .WithProtocolVersion(MqttProtocolVersion.V500)
            .Build());

        // foreach (IMqttEntity entity in _entities.Where(e => e is { Retain: true, HasReceivedFromMqtt: false }))
        // {
        //     _scheduler.Schedule(TimeSpan.FromSeconds(5), () =>
        //     {
        //         if (!entity.HasReceivedFromMqtt)
        //         {
        //             Logger.LogInformation("No MQTT data received for entity {entity} so we set to its initial value", entity.Id);
        //             entity.SetValueToInitialValue();
        //         }
        //     });
        // }
    }

    public void Add(IStringConfigEntity configEntity)
    {
        _entities.Add(configEntity);
        
        Config? config = _context.Configs.FirstOrDefault(c => c.Name == configEntity.Id);
        if (config == null && configEntity.Retain)
        {
            Logger.LogTrace("Add entity {entity} with value {value}", configEntity.Id, configEntity.ValueAsString());

            config = new Config
            {
                Name = configEntity.Id,
                Value = configEntity.ValueAsString()
            };
            
            _context.Add(config);
            _context.SaveChanges();
        }

        if (config != null)
        {
            configEntity.SetFromStringPayload(config.Value);
        }
        
        configEntity.ValueStringAsync()
            .SubscribeAsync(async value =>
            {
                Logger.LogTrace("Update {entityId} to {state}", configEntity.Id, value);

                if (config != null)
                {
                    config.Value = value;
                    await _context.SaveChangesAsync();
                }
                
                if (configEntity.Mqtt && _mqttClient.IsConnected)
                {
                    Logger.LogTrace("Send MQTT {entityId} to {state}", configEntity.Id, value);

                    MqttApplicationMessageBuilder mqttApplicationMessage = new MqttApplicationMessageBuilder()
                        .WithTopic($"{_topicBase}/{configEntity.Id}")
                        .WithPayload(value)
                        .WithRetainFlag(configEntity.Retain);

                    await _mqttClient.PublishAsync(mqttApplicationMessage.Build());
                }
            });
    }

    public async ValueTask DisposeAsync()
    {
        if (_mqttClient.IsConnected)
        {
            await _mqttClient.DisconnectAsync();
        }
    }
}

public record MqttEntities
{
    public ConfigEntity<bool> GpioWifi { get; } = new("gpio/wifi");
    public ConfigEntity<bool> GpioNpr { get; } = new("gpio/npr");
    public ConfigEntity<int> GpioBoxLdr { get; } = new("gpio/box_ldr");

    public ConfigEntity<bool> StatusBoxOpened { get; } = new("status/box_opened", false, default, true);

    public ConfigEntity<string> McuStatus { get; } = new("mcu/status", false, default, true);
    public ConfigEntity<float> TemperatureRtc { get; } = new("mcu/temperature_rtc", false, default, true);
    public ConfigEntity<TimeSpan> McuUptime { get; } = new("mcu/uptime", false, default, true);
    
    public ConfigEntity<bool> FeatureWatchdogSafetyEnabled { get; } = new("feature/watchdog_safety");
    public ConfigEntity<bool> FeatureAprsDigipeaterEnabled { get; } = new("feature/aprs_digipeater");
    public ConfigEntity<bool> FeatureAprsTelemetryEnabled { get; } = new("feature/aprs_telemetry");
    public ConfigEntity<bool> FeatureAprsPositionEnabled { get; } = new("feature/aprs_position");
    public ConfigEntity<bool> FeatureSleepEnabled { get; } = new("feature/sleep");

    public ConfigEntity<float> WeatherTemperature { get; } = new("weather/temperature", false, default, true);
    public ConfigEntity<float> WeatherHumidity { get; } = new("weather/humidity", false, default, true);
    public ConfigEntity<float> WeatherPressure { get; } = new("weather/pressure", false, default, true);

    public ConfigEntity<int> BatteryVoltage { get; } = new("battery/voltage", false, default, true);
    public ConfigEntity<int> BatteryCurrent { get; } = new("battery/current", false, default, true);
    public ConfigEntity<int> SolarVoltage { get; } = new("solar/voltage", false, default, true);
    public ConfigEntity<int> SolarCurrent { get; } = new("solar/current", false, default, true);
    public ConfigEntity<bool> SolarIsDay { get; } = new("solar/is_day", false, default, true);

    public ConfigEntity<int> MpptChargeCurrent { get; } = new("mppt/charge_current");
    public ConfigEntity<string> MpptStatus { get; } = new("mppt/status");
    public ConfigEntity<bool> MpptAlertShutdown { get; } = new("mppt/alert_shutdown", false, default, true);
    public ConfigEntity<bool> MpptPowerEnabled { get; } = new("mppt/power_enabled");
    public ConfigEntity<int> MpptPowerOffVoltage { get; } = new("mppt/power_off_voltage", true, 11500);
    public ConfigEntity<int> MpptPowerOnVoltage { get; } = new("mppt/power_on_voltage", true, 12500);
    public ConfigEntity<float> MpptTemperature { get; } = new("mppt/temperature", false, default, true);
    
    public ConfigEntity<bool> WatchdogEnabled { get; } = new("watchdog/enabled", false, default, true);
    public ConfigEntity<TimeSpan> WatchdogCounter { get; } = new("watchdog/counter");
    public ConfigEntity<TimeSpan> WatchdogPowerOffTime { get; } = new("watchdog/power_off_time");

    public ConfigEntity<string> LoraTxPayload { get; } = new("lora/tx_payload", false, default, true);

    public ConfigEntity<string> LoraRxPayload { get; } = new("lora/rx_payload", false, default, true);
    
    public ConfigEntity<TimeSpan> SystemUptime { get; } = new("system/uptime", false, default, true);
    public ConfigEntity<int> SystemCpu { get; } = new("system/cpu");
    public ConfigEntity<int> SystemRam { get; } = new("system/ram");
    public ConfigEntity<int> SystemDisk { get; } = new("system/disk", false, default, true);
}