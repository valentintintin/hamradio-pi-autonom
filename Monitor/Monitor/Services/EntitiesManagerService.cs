using System.Reactive.Concurrency;
using System.Reactive.Linq;
using System.Reflection;
using System.Text;
using Monitor.Extensions;
using Monitor.Models;
using MQTTnet;
using MQTTnet.Client;
using MQTTnet.Protocol;

namespace Monitor.Services;

public class EntitiesManagerService : AService
{
    private readonly IScheduler _scheduler;
    public static MqttEntities Entities { get; } = new();

    private readonly string _topicBase;
    private readonly IMqttClient _mqttClient;
    private readonly List<IMqttEntity> _entities = new();
    private readonly TimeSpan _durationBeforeMqtt = TimeSpan.FromSeconds(5);

    public EntitiesManagerService(ILogger<EntitiesManagerService> logger, 
        IConfiguration configuration, IServiceProvider serviceProvider) : base(logger)
    {
        _scheduler = serviceProvider.CreateScope().ServiceProvider.GetRequiredService<IScheduler>();

        IConfigurationSection configurationSection = configuration.GetSection("Mqtt");
        _topicBase = configurationSection.GetValueOrThrow<string>("TopicBase");

        
        MqttFactory mqttFactory = new();
        _mqttClient = mqttFactory.CreateMqttClient();

        foreach (PropertyInfo property in typeof(MqttEntities).GetProperties())
        {
            IMqttEntity? entity = (IMqttEntity?)property.GetValue(Entities);
            Add(entity ?? throw new InvalidOperationException());
        }
        
        MqttClientOptions mqttClientOptions = new MqttClientOptionsBuilder()
            .WithTcpServer(configurationSection.GetValueOrThrow<string>("Host"), configurationSection.GetValue("Port", 1883))
            .WithKeepAlivePeriod(TimeSpan.FromMinutes(1))
            .WithClientId("station")
            .Build();

        _mqttClient.ConnectedAsync += async _ =>
        {
            Logger.LogTrace("Connection successful to MQTT");
            
            await _mqttClient.SubscribeAsync(
                new MqttTopicFilterBuilder()
                    .WithTopic($"{_topicBase}/#")
                    .Build()
            );
        };

        _mqttClient.DisconnectedAsync += async args => 
        {
            logger.LogWarning(args.Exception, "MQTT disconnected");
            await Task.Delay(TimeSpan.FromSeconds(5));

            try
            {
                await _mqttClient.ConnectAsync(mqttClientOptions, CancellationToken.None);
            }
            catch (Exception e)
            {
                logger.LogWarning(e, "MQTT reconnecting failed");
            }
        };

        _mqttClient.ApplicationMessageReceivedAsync += MqttMessageReceived;

        _mqttClient.ConnectAsync(mqttClientOptions);
    }

    public void Add(IMqttEntity entity)
    {
        _entities.Add(entity);
        
        entity.ValueMqttAsync()
            .Where(v => !entity.HasReceivedInitialValueFromMqtt || v.old != v.value)
            .SubscribeAsync(async value =>
            {
                Logger.LogTrace("Update MQTT {entityId} from {oldState} to {state}", entity.Id, value.old, value.value);

            MqttApplicationMessageBuilder mqttApplicationMessage = new MqttApplicationMessageBuilder()
                .WithTopic($"{_topicBase}/{entity.Id}")
                .WithPayload(value.value)
                .WithRetainFlag(entity.Retain);
            
            await _mqttClient.PublishAsync(
                mqttApplicationMessage.Build(),
                CancellationToken.None);
        });

        _scheduler.Schedule(_durationBeforeMqtt, () =>
        {
            if (!entity.HasReceivedInitialValueFromMqtt)
            {
                entity.EmitToMqtt();
            }
        });
    }

    private Task MqttMessageReceived(MqttApplicationMessageReceivedEventArgs message)
    {
        string id = message.ApplicationMessage.Topic.Replace($"{_topicBase}/", "");
            
        IMqttEntity? entity = _entities.FirstOrDefault(entity => entity.Id == id);
        string payload = Encoding.UTF8.GetString(message.ApplicationMessage.PayloadSegment);

        if (entity != null)
        {
            try
            {
                if (entity.SetFromMqttPayload(payload))
                {
                    Logger.LogInformation("Change from MQTT entity {entityId} to {payload}", entity.Id, payload);
                }
            }
            catch (Exception e)
            {
                Logger.LogWarning(e, "Change from MQTT error entity {entityId} to {payload}", entity.Id, payload);
            }
        }
        else
        {
            Logger.LogInformation("From MQTT topic {topicId} and payload {payload} not found", id, payload);
        }
            
        return Task.CompletedTask;
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