using System.Reactive.Concurrency;
using Monitor.Extensions;
using Monitor.WorkServices;
using NetDaemon.AppModel;
using NetDaemon.HassModel;

namespace Monitor.Apps;

[NetDaemonApp(Id = "camera_capture_app")]
public class CameraCaptureApp : AApp
{
    private readonly CameraService _cameraService;

    public CameraCaptureApp(IHaContext ha, ILogger<CameraCaptureApp> logger, IConfiguration configuration,
        EntitiesManagerService entitiesManagerService, IScheduler scheduler, CameraService cameraService)
        : base(ha, logger, entitiesManagerService)
    {
        _cameraService = cameraService;
        TimeSpan interval = TimeSpan.FromSeconds(configuration.GetSection("Cameras").GetValueOrThrow<int>("Time"));

        Logger.LogInformation("Capture image every {interval}", interval);

        scheduler.ScheduleAsync(TimeSpan.FromSeconds(1), async (_, _) =>
        {
            await Do();
        });

        scheduler.SchedulePeriodic(interval, async () =>
        {
            await Do();
        });
    }

    private async Task Do()
    {
        await _cameraService.CaptureAllCameras();
        _cameraService.CreateFinalImageFromLasts();
    }
}