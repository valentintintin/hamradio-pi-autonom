using System.Reactive.Linq;
using Monitor.Extensions;
using Monitor.Models;
using Monitor.Services;

namespace Monitor.Workers;

public class MpptApp : AWorker
{
    public readonly MqttEntity<TimeSpan> DurationAfterAlert = new("sleep/duration_after_alert", true, TimeSpan.FromSeconds(20));

    private readonly SystemService _systemService;
    private readonly SerialMessageService _serialMessageService;

    public MpptApp(ILogger<MpptApp> logger, IServiceProvider serviceProvider) : base(logger, serviceProvider)
    {
        _systemService = Services.GetRequiredService<SystemService>();
        _serialMessageService = Services.GetRequiredService<SerialMessageService>();

        EntitiesManagerService.Add(DurationAfterAlert);
    }

    protected override Task Start()
    {
        MqttEntity<int> powerOnVoltageEntity = EntitiesManagerService.Entities.MpptPowerOnVoltage;
        MqttEntity<int> powerOffVoltageEntity = EntitiesManagerService.Entities.MpptPowerOffVoltage;

        AddDisposable(EntitiesManagerService.Entities.MpptAlertShutdown.ValueChanges()
            .Select(v => v.value)
            .Do(_ => Logger.LogWarning("Alert so shutdown in {duration}", DurationAfterAlert.Value))
            .Delay(DurationAfterAlert.Value)
            .Subscribe(_ =>
            {
                _systemService.Shutdown();
            }));

        AddDisposable(powerOffVoltageEntity.ValueChanges()
            .Merge(powerOnVoltageEntity.ValueChanges())
            .Subscribe(_ =>
            {
                _serialMessageService.SetPowerOnOffVoltage(
                    powerOnVoltageEntity.Value,
                    powerOffVoltageEntity.Value
                );
            }));
        
        return Task.CompletedTask;
    }
}