using Monitor.Extensions;
using Monitor.Models.SerialMessages;
using Monitor.WorkServices;
using Timer = System.Timers.Timer;

namespace Monitor.Workers;

public class MpptWorker : AWorker
{
    private readonly SerialMessageService _serialMessageService;
    private readonly SystemService _systemService;
    private readonly Timer _timer = new(10_000);
    
    public MpptWorker(ILogger<MpptWorker> logger, IConfiguration configuration, IServiceScopeFactory serviceScopeFactory) :
        base("Mppt", logger, configuration, serviceScopeFactory)
    {
        _serialMessageService = ServiceProvider.GetRequiredService<SerialMessageService>();
        _systemService = ServiceProvider.GetRequiredService<SystemService>();
    }

    protected override Task Start()
    {
        _timer.Elapsed += (_, _) =>
        {
            MpptData mppt = MonitorService.State.Mppt;
            
            if (mppt.Alert)
            {
                Logger.LogInformation("Mppt alert triggered");
                _systemService.Shutdown();

                return;
            }

            MonitorService.ComputeBattery();
            
            if (!_systemService.WillShutdown() && ConfigurationSection.GetValueOrThrow<bool>("WatchdogEnabled"))
            {
                Logger.LogTrace("Feed dog");
                
                _serialMessageService.SetWatchdog(TimeSpan.FromSeconds(255));
            }
        };
        _timer.Start();

        return Task.CompletedTask;
    }

    public override void Dispose()
    {
        _timer.Stop();
        _timer.Dispose();
        
        Logger.LogInformation("Stop watchdog");
        _serialMessageService.SetWatchdog(TimeSpan.Zero);
        
        base.Dispose();
    }
}