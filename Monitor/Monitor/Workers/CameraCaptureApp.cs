using System.Reactive.Concurrency;
using Monitor.Extensions;
using Monitor.Models;
using Monitor.Services;

namespace Monitor.Workers;

public class CameraCaptureApp : AEnabledWorker
{
    public readonly StringConfigEntity<TimeSpan> Interval = new("cameras/interval", true, TimeSpan.FromSeconds(30));
    
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
        _scheduler?.Dispose();

        try
        {
            await _cameraService.CreateFinalImageFromLasts();
        }
        catch (Exception e)
        {
            Logger.LogError(e, "Error during Camera Capture");
        }
        finally
        {
            _scheduler = AddDisposable(Scheduler.SchedulePeriodic(Interval.Value, async () => { await Do(); }));
        }
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