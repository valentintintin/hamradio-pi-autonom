using System.Reactive.Concurrency;
using Monitor.Extensions;
using Monitor.Models;
using Monitor.Services;

namespace Monitor.Workers;

public class CameraCaptureApp : AEnabledWorker
{
    public readonly MqttEntity<TimeSpan> Interval = new("cameras/interval", true, TimeSpan.FromSeconds(30));
    
    private readonly CameraService _cameraService;

    private IDisposable? _scheduler;

    public CameraCaptureApp(ILogger<CameraCaptureApp> logger, IServiceProvider serviceProvider)
        : base(logger, serviceProvider)
    {
        _cameraService = Services.GetRequiredService<CameraService>();
        EntitiesManagerService.Add(Interval);
    }

    private async Task Do()
    {
        await _cameraService.CreateFinalImageFromLasts();
            
        _scheduler?.Dispose();
        
        _scheduler = AddDisposable(Scheduler.SchedulePeriodic(Interval.Value, async () =>
        {
            await Do();
        }));
    }

    protected override async Task Start()
    {
        await Do();

        AddDisposable(Interval.ValueChanges().SubscribeAsync(async value =>
        {
            _scheduler?.Dispose();

            await Do();
        }));
    }
}