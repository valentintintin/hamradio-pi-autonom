using System.Reactive.Linq;
using Monitor.Extensions;
using Monitor.Models;
using Monitor.Services;

namespace Monitor.Workers;

public class MpptApp : AWorker
{
    private readonly SystemService _systemService;
    private readonly SerialMessageService _serialMessageService;

    public MpptApp(ILogger<MpptApp> logger, IServiceProvider serviceProvider) : base(logger, serviceProvider)
    {
        _systemService = Services.GetRequiredService<SystemService>();
        _serialMessageService = Services.GetRequiredService<SerialMessageService>();
    }

    protected override Task Start()
    {
        var powerOnVoltageConfigEntity = EntitiesManagerService.Entities.MpptPowerOnVoltage;
        var powerOffVoltageConfigEntity = EntitiesManagerService.Entities.MpptPowerOffVoltage;

        AddDisposable(EntitiesManagerService.Entities.MpptAlertShutdown.ValueChanges()
            .Select(v => v.value)
            .Do(_ => Logger.LogWarning("Alert so shutdown"))
            .SubscribeAsync(async _ =>
            {
                await _systemService.Shutdown();
            }));

        AddDisposable(powerOffVoltageConfigEntity.ValueChanges()
            .Merge(powerOnVoltageConfigEntity.ValueChanges())
            .Subscribe(_ =>
            {
                _serialMessageService.SetPowerOnOffVoltage(
                    powerOnVoltageConfigEntity.Value,
                    powerOffVoltageConfigEntity.Value
                );
            }));
        
        return Task.CompletedTask;
    }
}