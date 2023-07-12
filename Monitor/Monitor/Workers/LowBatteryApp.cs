using System.Reactive.Linq;
using Monitor.Models;
using Monitor.Services;

namespace Monitor.Workers;

public class LowBatteryApp : AWorker
{
    public static readonly MqttEntity<int> Voltage = new("low_battery/voltage", true, 11600);
    public static readonly MqttEntity<TimeSpan> TimeOff = new("low_battery/time_off", true, TimeSpan.FromMinutes(30));

    private readonly SystemService _systemService;
    
    public LowBatteryApp(ILogger<LowBatteryApp> logger, IServiceProvider serviceProvider) 
        : base(logger, serviceProvider)
    {
        EntitiesManagerService.Add(Voltage);
        EntitiesManagerService.Add(TimeOff);

        _systemService = Services.GetRequiredService<SystemService>();
    }

    protected override Task Start()
    {
        AddDisposable(EntitiesManagerService.Entities.BatteryVoltage.ValueChanges()
            .Select(v => v.value)
            .Buffer(TimeSpan.FromMinutes(2))
            .Select(s => s.Any() ? s.Average() : int.MaxValue)
            .Do(s => Logger.LogDebug("Average Battery Voltage : {averageVoltage}", s))
            .Where(s => s < Voltage.Value)
            .Subscribe(s =>
            {
                Logger.LogWarning("Battery is too low so sleep. {batteryVoltage} < {lowVoltage}", s, Voltage.Value);

                _systemService.AskForShutdown(TimeOff.Value);
            }));
        
        return Task.CompletedTask;
    }
}