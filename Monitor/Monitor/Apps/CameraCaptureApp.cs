using System.Reactive.Concurrency;
using Monitor.Extensions;
using Monitor.WorkServices;
using NetDaemon.AppModel;
using NetDaemon.HassModel;
using NetDaemon.HassModel.Entities;

namespace Monitor.Apps;

[NetDaemonApp(Id = "camera_capture_app")]
public class CameraCaptureApp : AApp, IAsyncDisposable, IAsyncInitializable
{
    public CameraCaptureApp(IHaContext ha, ILogger<CameraCaptureApp> logger, IConfiguration configuration,
        EntitiesManagerService entitiesManagerService, IScheduler scheduler, CameraService cameraService)
        : base(ha, logger, entitiesManagerService)
    {
        TimeSpan interval = TimeSpan.FromSeconds(configuration.GetSection("Cameras").GetValueOrThrow<int>("Time"));

        Logger.LogInformation("Capture image every {interval}", interval);
        
        scheduler.SchedulePeriodic(interval, () =>
        {
            cameraService.CreateFinalImageFromLasts();
        });
        
        SwitchStillImage(true);
    }

    public Task InitializeAsync(CancellationToken cancellationToken)
    {
        if (EntitiesManagerService.Entities.MotionEye?.IsOff(Logger) == true)
        {
            Logger.LogInformation("Start motioneye docker");

            EntitiesManagerService.Entities.MotionEye.TurnOn(Logger);
        }

        return Task.CompletedTask;
    }

    public ValueTask DisposeAsync()
    {
        SwitchStillImage(false);

        GC.SuppressFinalize(this);
        
        return ValueTask.CompletedTask;
    }

    private void SwitchStillImage(bool state)
    {
        Logger.LogInformation("Chnge motioneye capture to {state}", state);
        
        List<Entity> camerasStillImages = EntitiesManagerService.Entities.CamerasStillImage;

        foreach (Entity cameraStillImages in camerasStillImages)
        {
            cameraStillImages.Turn(state, Logger);
        }
    }
}