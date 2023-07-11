using System.Reactive.Concurrency;
using Monitor.Extensions;
using Monitor.Models;
using Monitor.Services;

namespace Monitor.Workers;

public class CameraCaptureApp : AWorker
{
    public readonly MqttEntity<TimeSpan> Interval = new("cameras/interval", true, TimeSpan.FromSeconds(30));
    
    private readonly CameraService _cameraService;

    private IDisposable? _scheduler;

    public CameraCaptureApp(ILogger<CameraCaptureApp> logger, IServiceProvider serviceProvider)
        : base(logger, serviceProvider)
    {
        _cameraService = Services.GetRequiredService<CameraService>();
    }

    private async Task Do()
    {
        await _cameraService.CreateFinalImageFromLasts();
    }

    protected override async Task Start()
    {
        await Do();

        AddDisposable(Interval.ValueChanges().Subscribe(value =>
        {
            _scheduler?.Dispose();
            
            _scheduler = AddDisposable(Scheduler.SchedulePeriodic(value.value, async () =>
            {
                await Do();
            }));
        }));
    }
}