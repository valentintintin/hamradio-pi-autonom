using Monitor.Extensions;
using Monitor.Services;
using Timer = System.Timers.Timer;

namespace Monitor.Workers;

public class WebcamWorker : AWorker
{
    private readonly Timer _timer;
    private readonly string _webcamUrl;
    private readonly CameraService _cameraService;

    public WebcamWorker(ILogger<WebcamWorker> logger, IConfiguration configuration,
        IServiceScopeFactory serviceScopeFactory) :
        base("Webcam", logger, configuration, serviceScopeFactory)
    {
        _webcamUrl = ConfigurationSection.GetValueOrThrow<string>("Url");
        _timer = new Timer(ConfigurationSection.GetValueOrThrow<int>("Time"));
        _cameraService = ServiceProvider.GetRequiredService<CameraService>();
    }

    protected override Task Start()
    {
        _timer.Elapsed += async (_, _) =>
        {
            await _cameraService.Capture(_webcamUrl);
        };
        
        _timer.Start();
        
        return Task.CompletedTask;
    }

    public override void Dispose()
    {
        _timer.Stop();
        _timer.Dispose();
    }
}