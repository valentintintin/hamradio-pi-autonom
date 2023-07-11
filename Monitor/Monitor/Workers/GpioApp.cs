using System.Reactive.Concurrency;
using System.Reactive.Linq;
using Monitor.Models;
using Monitor.Services;

namespace Monitor.Workers;

public class GpioApp : AWorker
{
    public static readonly MqttEntity<bool> StartWifiTurnOn = new("gpio/start_wifi_turn_on", true);
    public static readonly MqttEntity<bool> StartNprTurnOn = new("gpio/start_npr_turn_on", true);
    
    private readonly SerialMessageService _serialMessageService;
    private readonly TimeSpan _debunce = TimeSpan.FromSeconds(1);

    public GpioApp(ILogger<GpioApp> logger, IServiceProvider serviceProvider) 
        : base(logger, serviceProvider)
    {
        _serialMessageService = Services.GetRequiredService<SerialMessageService>();

        EntitiesManagerService.Add(StartWifiTurnOn);
        EntitiesManagerService.Add(StartNprTurnOn);
    }

    protected override Task Start()
    {
        AddDisposable(EntitiesManagerService.Entities.GpioWifi.ValueChanges().Skip(1)
            .Sample(_debunce)
            .Select(v => v.value)
            .Subscribe(_serialMessageService.SetWifi)
        );
        
        AddDisposable(EntitiesManagerService.Entities.GpioNpr.ValueChanges().Skip(1)
            .Sample(_debunce)
            .Select(v => v.value)
            .Subscribe(_serialMessageService.SetNpr)
        );

        AddDisposable(Scheduler.Schedule(TimeSpan.FromSeconds(5), _ =>
        {
            Logger.LogInformation("Switch GPIOs");
            
            EntitiesManagerService.Entities.GpioWifi.SetValue(StartWifiTurnOn.Value);
            EntitiesManagerService.Entities.GpioNpr.SetValue(StartNprTurnOn.Value);
        }));
        
        return Task.CompletedTask;
    }
}