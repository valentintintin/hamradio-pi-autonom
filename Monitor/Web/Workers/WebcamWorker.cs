using Web.Extensions;
using Web.Services;
using static Web.Services.FileDownloaderService;
using Timer = System.Timers.Timer;

namespace Web.Workers;

public class WebcamWorker : IHostedService, IDisposable
{
    private readonly ILogger<WebcamWorker> _logger;
    private readonly FileDownloaderService _fileDownloaderService;
    private Timer _timer;
    private string? webcamUrl { get; }

    public WebcamWorker(ILogger<WebcamWorker> logger, IConfiguration configuration,
        FileDownloaderService fileDownloaderService)
    {
        _logger = logger;
        _fileDownloaderService = fileDownloaderService;

        IConfigurationSection configurationSection = configuration.GetSection("Webcam");
        
        webcamUrl = configurationSection.GetValueOrThrow<string>("Url");
        _timer = new Timer(configurationSection.GetValueOrThrow<int>("Time"));
        
        _logger.LogTrace("Webcam timer set to {timer}ms", _timer.Interval);
    }
    
    public Task StartAsync(CancellationToken cancellationToken)
    {
        _timer.Elapsed += async (_, _) =>
        {
            string filePath = $"storage/camera/{DateTime.UtcNow:u}.jpg";
            _logger.LogInformation("Get webcam snapshot from {webcamUrl} to {filePath}", webcamUrl, filePath);
            try
            {
                await _fileDownloaderService.DownloadFileAsync(webcamUrl, filePath);
            }
            catch (Exception e)
            {
                _logger.LogError(e, "Get webcam snapshot KO");
            }
        };
        _timer.Start();
        return Task.CompletedTask;
    }

    public Task StopAsync(CancellationToken cancellationToken)
    {
        return Task.CompletedTask;
    }

    public void Dispose()
    {
        _timer.Stop();
        _timer.Dispose();
    }
}