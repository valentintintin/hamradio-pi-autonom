using System.Collections;
using System.Globalization;
using System.Reactive.Concurrency;
using System.Reactive.Linq;
using System.Reflection;
using System.Text;
using Microsoft.EntityFrameworkCore.ChangeTracking;
using Monitor.Exceptions;
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
    private readonly List<IMqttEntity> _entities = new();
    private readonly IConfigurationSection _configurationSection;
    private readonly IScheduler _scheduler;
    
    public EntitiesManagerService(ILogger<EntitiesManagerService> logger, 
        IConfiguration configuration, IServiceProvider serviceProvider) : base(logger)
    {
        _configurationSection = configuration.GetSection("Mqtt");
        _topicBase = _configurationSection.GetValueOrThrow<string>("TopicBase");
        _clientId = _configurationSection.GetValueOrThrow<string>("ClientId");
        _mqttClient = new MqttFactory().CreateMqttClient();
        _scheduler = serviceProvider.CreateScope().ServiceProvider.GetRequiredService<IScheduler>();
        
        foreach (PropertyInfo property in typeof(MqttEntities).GetProperties())
        {
            IMqttEntity? entity = (IMqttEntity?)property.GetValue(Entities);
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
        
        Observable.FromEvent<Func<MqttApplicationMessageReceivedEventArgs, Task>, MqttApplicationMessageReceivedEventArgs>(
                handler => args =>
                {
                    handler(args);
                    return Task.CompletedTask;
                },
                h => _mqttClient.ApplicationMessageReceivedAsync += h,
                h => _mqttClient.ApplicationMessageReceivedAsync -= h
            )
            .Select(m =>
            {
                string id = m.ApplicationMessage.Topic.Replace($"{_topicBase}/", "");
            
                IMqttEntity? entity = _entities.FirstOrDefault(entity => entity.Id == id);
                string payload = Encoding.UTF8.GetString(m.ApplicationMessage.PayloadSegment);

                if (entity != null)
                {
                    return new
                    {
                        entity,
                        payload
                    };
                }

                Logger.LogError("From MQTT topic {topicId} not found with payload {payload}", id, payload);
                return null;
            })
            .Where(m => m != null)
            .Subscribe(m =>
            {
                try
                {
                    if (m!.entity.SetFromMqttPayload(m.payload))
                    {
                        Logger.LogInformation("Change from MQTT entity {entityId} to {payload}", m.entity.Id, m.payload);
                    }
                }
                catch (Exception e)
                {
                    Logger.LogError(e, "Change from MQTT error entity {entityId} to {payload}", m!.entity.Id, m.payload);
                }
            });
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

        foreach (IMqttEntity entity in _entities.Where(e => e is { Retain: true, HasReceivedFromMqtt: false }))
        {
            _scheduler.Schedule(TimeSpan.FromSeconds(5), () =>
            {
                if (!entity.HasReceivedFromMqtt)
                {
                    Logger.LogInformation("No MQTT data received for entity {entity} so we set to its initial value", entity.Id);
                    entity.SetValueToInitialValue();
                }
            });
        }
    }

    public void Add(IMqttEntity entity)
    {
        Logger.LogTrace("Add MQTT entity {entity}", entity.Id);
        _entities.Add(entity);
        
        entity.ValueMqttAsync()
            .Where(_ => _mqttClient.IsConnected)
            .SubscribeAsync(async value =>
            {
                Logger.LogTrace("Update MQTT {entityId} to {state}", entity.Id, value);

                MqttApplicationMessageBuilder mqttApplicationMessage = new MqttApplicationMessageBuilder()
                    .WithTopic($"{_topicBase}/{entity.Id}")
                    .WithPayload(value)
                    .WithRetainFlag(entity.Retain);
                
                await _mqttClient.PublishAsync(mqttApplicationMessage.Build());
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
    public MqttEntity<bool> GpioWifi { get; } = new("gpio/wifi");
    public MqttEntity<bool> GpioNpr { get; } = new("gpio/npr");
    public MqttEntity<int> GpioBoxLdr { get; } = new("gpio/box_ldr");

    public MqttEntity<bool> StatusBoxOpened { get; } = new("status/box_opened");

    public MqttEntity<string> McuStatus { get; } = new("mcu/status");
    public MqttEntity<TimeSpan> McuUptime { get; } = new("mcu/uptime");

    public MqttEntity<float> WeatherTemperature { get; } = new("weather/temperature");
    public MqttEntity<int> WeatherHumidity { get; } = new("weather/humidity");

    public MqttEntity<int> BatteryVoltage { get; } = new("battery/voltage");
    public MqttEntity<int> BatteryCurrent { get; } = new("battery/current");
    public MqttEntity<int> SolarVoltage { get; } = new("solar/voltage");
    public MqttEntity<int> SolarCurrent { get; } = new("solar/current");
    public MqttEntity<bool> SolarIsDay { get; } = new("solar/is_day");

    public MqttEntity<int> MpptChargeCurrent { get; } = new("mppt/charge_current");
    public MqttEntity<string> MpptStatus { get; } = new("mppt/status");
    public MqttEntity<bool> MpptAlertShutdown { get; } = new("mppt/alert_shutdown");
    public MqttEntity<bool> MpptPowerEnabled { get; } = new("mppt/power_enabled");
    public MqttEntity<int> MpptPowerOffVoltage { get; } = new("mppt/power_off_voltage", true, 11500);
    public MqttEntity<int> MpptPowerOnVoltage { get; } = new("mppt/power_on_voltage", true, 12500);
    
    public MqttEntity<bool> WatchdogEnabled { get; } = new("watchdog/enabled");
    public MqttEntity<TimeSpan> WatchdogCounter { get; } = new("watchdog/counter");
    public MqttEntity<TimeSpan> WatchdogPowerOffTime { get; } = new("watchdog/power_off_time");

    public MqttEntity<string> LoraTxPayload { get; } = new("lora/tx_payload");

    public MqttEntity<string> LoraRxPayload { get; } = new("lora/rx_payload");
    
    public MqttEntity<TimeSpan> SystemUptime { get; } = new("system/uptime");
    public MqttEntity<int> SystemCpu { get; } = new("system/cpu");
    public MqttEntity<int> SystemRam { get; } = new("system/ram");
    public MqttEntity<int> SystemDisk { get; } = new("system/disk");
}