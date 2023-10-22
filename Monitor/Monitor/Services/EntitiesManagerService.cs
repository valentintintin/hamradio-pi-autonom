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

                if (_mqttClient.IsConnected)
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
    public StringConfigEntity<bool> GpioWifi { get; } = new("gpio/wifi");
    public StringConfigEntity<bool> GpioNpr { get; } = new("gpio/npr");
    public StringConfigEntity<int> GpioBoxLdr { get; } = new("gpio/box_ldr");

    public StringConfigEntity<bool> StatusBoxOpened { get; } = new("status/box_opened");

    public StringConfigEntity<string> McuStatus { get; } = new("mcu/status");
    public StringConfigEntity<TimeSpan> McuUptime { get; } = new("mcu/uptime");

    public StringConfigEntity<float> WeatherTemperature { get; } = new("weather/temperature");
    public StringConfigEntity<float> WeatherHumidity { get; } = new("weather/humidity");
    public StringConfigEntity<float> WeatherPressure { get; } = new("weather/pressure");

    public StringConfigEntity<int> BatteryVoltage { get; } = new("battery/voltage");
    public StringConfigEntity<int> BatteryCurrent { get; } = new("battery/current");
    public StringConfigEntity<int> SolarVoltage { get; } = new("solar/voltage");
    public StringConfigEntity<int> SolarCurrent { get; } = new("solar/current");
    public StringConfigEntity<bool> SolarIsDay { get; } = new("solar/is_day");

    public StringConfigEntity<int> MpptChargeCurrent { get; } = new("mppt/charge_current");
    public StringConfigEntity<string> MpptStatus { get; } = new("mppt/status");
    public StringConfigEntity<bool> MpptAlertShutdown { get; } = new("mppt/alert_shutdown");
    public StringConfigEntity<bool> MpptPowerEnabled { get; } = new("mppt/power_enabled");
    public StringConfigEntity<int> MpptPowerOffVoltage { get; } = new("mppt/power_off_voltage", true, 11500);
    public StringConfigEntity<int> MpptPowerOnVoltage { get; } = new("mppt/power_on_voltage", true, 12500);
    
    public StringConfigEntity<bool> WatchdogEnabled { get; } = new("watchdog/enabled");
    public StringConfigEntity<TimeSpan> WatchdogCounter { get; } = new("watchdog/counter");
    public StringConfigEntity<TimeSpan> WatchdogPowerOffTime { get; } = new("watchdog/power_off_time");

    public StringConfigEntity<string> LoraTxPayload { get; } = new("lora/tx_payload");

    public StringConfigEntity<string> LoraRxPayload { get; } = new("lora/rx_payload");
    
    public StringConfigEntity<TimeSpan> SystemUptime { get; } = new("system/uptime");
    public StringConfigEntity<int> SystemCpu { get; } = new("system/cpu");
    public StringConfigEntity<int> SystemRam { get; } = new("system/ram");
    public StringConfigEntity<int> SystemDisk { get; } = new("system/disk");
}