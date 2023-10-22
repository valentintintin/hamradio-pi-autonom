using System.Reactive.Concurrency;
using System.Reactive.Linq;
using Monitor.Models;
using Monitor.Services;

namespace Monitor.Workers;

public class GpioApp : AWorker
{
    public readonly StringConfigEntity<bool> StartWifiTurnOn = new("gpio/start_wifi_turn_on", true);
    public readonly StringConfigEntity<bool> StartNprTurnOn = new("gpio/start_npr_turn_on", true);
    
    private readonly SerialMessageService _serialMessageService;

    public GpioApp(ILogger<GpioApp> logger, IServiceProvider serviceProvider) 
        : base(logger, serviceProvider)
    {
        _serialMessageService = Services.GetRequiredService<SerialMessageService>();

        EntitiesManagerService.Add(StartWifiTurnOn);
        EntitiesManagerService.Add(StartNprTurnOn);
    }

    protected override Task Start()
    {
        AddDisposable(EntitiesManagerService.Entities.GpioWifi.ValueChanges()
            .Sample(TimeSpan.FromSeconds(1))
            .Do(v => Logger.LogDebug("GPIO wifi => {value}", v))
            .Select(v => v.value)
            .Subscribe(_serialMessageService.SetWifi)
        );
        
        AddDisposable(EntitiesManagerService.Entities.GpioNpr.ValueChanges()
            .Sample(TimeSpan.FromSeconds(1))
            .Do(v => Logger.LogDebug("GPIO npr => {value}", v))
            .Select(v => v.value)
            .Subscribe(_serialMessageService.SetNpr)
        );

        Observable.Timer(TimeSpan.FromSeconds(2)).Select(_ => true)
            .Merge(
                EntitiesManagerService.Entities.McuStatus.ValueChanges()
                .Select(v => v.value)
                .Where(v => v is "starting" or "started")
                .Select(_ => true)
                .Do(v => Logger.LogInformation("Will switch GPIO because MCU in state {state}", v))
                .Delay(TimeSpan.FromSeconds(30)) // Wait init done
            )
            .Merge(StartWifiTurnOn.ValueChanges().Where(s => s.value).Select(v => v.value))
            .Merge(StartNprTurnOn.ValueChanges().Where(s => s.value).Select(v => v.value))
            .Subscribe(_ => 
        {
            Logger.LogInformation("Switch GPIO with start values");
        
            if (StartWifiTurnOn.Value != EntitiesManagerService.Entities.GpioWifi.Value)
            {
                EntitiesManagerService.Entities.GpioWifi.SetValue(StartWifiTurnOn.Value);
            }
        
            if (StartNprTurnOn.Value != EntitiesManagerService.Entities.GpioNpr.Value)
            {
                EntitiesManagerService.Entities.GpioNpr.SetValue(StartNprTurnOn.Value);
            }
        });

        return Task.CompletedTask;
    }
}