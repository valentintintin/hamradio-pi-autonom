using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.Reflection;
using Monitor.Exceptions;
using Monitor.Extensions;
using Monitor.Models;

namespace Monitor.Services;

public class CameraService : AService
{
    private bool _isRunning;

    private readonly string _storagePath;
    private readonly string? _message;
    private readonly List<FswebcamParameters> _fswebcamParameters;

    private readonly string _fontPath = Path.Combine(Path.GetDirectoryName(Assembly.GetEntryAssembly()!.Location)!, "Arial.ttf");

    private const int FrameToTakeDuringNight = 25;
    private const int WidthData = 700;
    private const int LegendSize = 70;
    private const int LegendMargin = 15;
    private const int HeightProgressBar = 25;
    private readonly int _widthMaxProgressBar;

    public CameraService(ILogger<CameraService> logger, IConfiguration configuration) : base(logger)
    {
        var configurationSection = configuration.GetSection("Cameras");
        _message = configurationSection.GetValue<string?>("Message");
        _fswebcamParameters = configurationSection.GetSection("Devices").Get<List<FswebcamParameters>>()?.ToList() ?? new List<FswebcamParameters>();
        
        _storagePath = Path.Combine(
            configuration.GetValueOrThrow<string>("StoragePath"), 
            configurationSection.GetValueOrThrow<string>("Path")
        );
        Directory.CreateDirectory($"{_storagePath}");
        
        _widthMaxProgressBar = WidthData - (LegendSize + LegendMargin) * 2;
    }

    public string? GetFinalLast()
    {
        var path = $"{_storagePath}/last.jpg";

        if (!File.Exists(path))
        {
            Logger.LogWarning("Last final image does not exist at {path}", path);
            
            return null;
        }
        
        var resolveLinkTarget = File.ResolveLinkTarget(path, true);

        if (resolveLinkTarget == null)
        {
            return path;
        }

        return resolveLinkTarget?.Exists == true ? resolveLinkTarget.FullName : null;
    }

    public async Task<MemoryStream> CreateFinalImageFromLasts(bool save = true)
    {
        Logger.LogInformation("Create final image");

        if (_isRunning)
        {
            throw new WarningException("Already running");
        }

        _isRunning = true;
        
        var now = DateTime.UtcNow;
        ImageMagickParameters imageMagickParameters = new();

        try
        {
            List<(string path, FswebcamParameters parameters)> imagesCamera = CaptureAllCameras();

            Logger.LogDebug("We have {count} photos", imagesCamera.Count);

            var width = 0;
            var height = 480;

            if (imagesCamera.Any())
            {
                width = imagesCamera.Sum(i => i.parameters.Width);
                height = imagesCamera.Max(i => i.parameters.Height);
            }

            imageMagickParameters.CreateCanvas(width + WidthData, height, "black");
            
            foreach ((string path, FswebcamParameters parameters) image in imagesCamera)
            {
                imageMagickParameters.OverlayImage(image.path, 0, 0).Translate(image.parameters.Width, 0);
            }

            imageMagickParameters.AddText(30, _fontPath, LegendMargin + LegendSize + 100, 30, $"{now.ToFrench():G}", "white");

            DrawProgressBarwithInfo(imageMagickParameters, 0, "Tension batterie", "mV", EntitiesManagerService.Entities.BatteryVoltage.Value, 11000, 13500, "indianred");
            DrawProgressBarwithInfo(imageMagickParameters, 1, "Courant batterie", "mA", EntitiesManagerService.Entities.BatteryCurrent.Value, 0, 1000, "cyan");
            DrawProgressBarwithInfo(imageMagickParameters, 2, "Tension panneau", "mV", EntitiesManagerService.Entities.SolarVoltage.Value, 0, 25000, "yellow");
            DrawProgressBarwithInfo(imageMagickParameters, 3, "Courant panneau", "mA", EntitiesManagerService.Entities.SolarCurrent.Value, 0, 4000, "lightgreen");
            DrawProgressBarwithInfo(imageMagickParameters, 4, "Température", "°C", EntitiesManagerService.Entities.WeatherTemperature.Value, -10, 40, "darkorange");
            DrawProgressBarwithInfo(imageMagickParameters, 5, "Humidité", "%", EntitiesManagerService.Entities.WeatherHumidity.Value, 0, 100, "lightblue");
            DrawProgressBarwithInfo(imageMagickParameters, 6, "Pression", "hPa", EntitiesManagerService.Entities.WeatherPressure.Value, 950, 1050, "lightpink");
            
            if (!string.IsNullOrWhiteSpace(_message))
            {
                imageMagickParameters.AddText(15, _fontPath, WidthData - 15 * _message.Length, height - 30, _message, "white");
            }

            var tempOutputFile = $"{_storagePath}/output.jpg";
            imageMagickParameters.SetQuality(80).SetFormat("jpeg").Output(tempOutputFile);

            ImageMagick(imageMagickParameters);

            MemoryStream stream = new();

            await using (FileStream fileStream = new(tempOutputFile, FileMode.Open, FileAccess.Read))
            {
                await fileStream.CopyToAsync(stream);
                stream.Seek(0, SeekOrigin.Begin);
            }
            
            Logger.LogDebug("Saving image to stream. Size : {size}", stream.Length);
            
            if (save)
            {
                Directory.CreateDirectory($"{_storagePath}/{now:yyyy-MM-dd}");

                var filePath =
                    $"{_storagePath}/{now:yyyy-MM-dd}/{now:yyyy-MM-dd-HH-mm-ss}-{Random.Shared.NextInt64()}.jpg";
                var lastPath = $"{_storagePath}/last.jpg";

                Logger.LogDebug("Save final image to {path}", filePath);

                await using var fileStream = File.Create(filePath);
                await stream.CopyToAsync(fileStream);

                File.Delete(lastPath);
                File.CreateSymbolicLink(lastPath, filePath);
            }

            Logger.LogInformation("Create final image OK");
            
            return stream;
        }
        finally
        {
            _isRunning = false;
        }
    }

    private List<(string path, FswebcamParameters parameters)> CaptureAllCameras()
    {
        Logger.LogInformation("Will capture images from cameras : {cameras}", _fswebcamParameters.Select(j => j.Device).JoinString());

        return _fswebcamParameters.Select(parameters =>
        {
            var cameraName = parameters.Device
                .Replace("/dev/v4l/by-id/", string.Empty)
                .Replace("/dev/video_", string.Empty)
                .Replace("/dev/video", string.Empty);

            Logger.LogDebug("Capture image from {camera}", cameraName);

            parameters.Frames = MonitorService.State.Mppt.Night ? FrameToTakeDuringNight : null;
            parameters.SaveFile = $"{_storagePath}/{cameraName}.jpg";
            
            return (CaptureImage(parameters), parameters);
        }).ToList()!;
    }
    
    private void DrawProgressBarwithInfo(ImageMagickParameters parameters, int indexValue, string label, string unit, double value, double min, double max, string colorBar)
    {
        var y = indexValue * 50 + 80;
        var valueBar = value;
        
        if (valueBar > max)
        {
            valueBar = max;
        } 
        else if (value < min)
        {
            valueBar = min;
        }

        valueBar = (int)Math.Round((double)MapValue(valueBar, min, max, 0, _widthMaxProgressBar), 2);
        
        parameters
            .DrawRectangle(LegendMargin + LegendSize, y + 5, _widthMaxProgressBar, HeightProgressBar, "lightgray")
            .DrawRectangle(LegendMargin + LegendSize, y + 5, (int) valueBar, HeightProgressBar, colorBar)
            .AddText(20, _fontPath, LegendMargin + LegendSize + 50, y, $"{label} : {value} {unit}", "black")
            .AddText(20, _fontPath, LegendMargin, y, $"{min.ToString(CultureInfo.CurrentCulture)}", colorBar)
            .AddText(20, _fontPath, LegendMargin + LegendSize + _widthMaxProgressBar + 5, y, $"{max.ToString(CultureInfo.CurrentCulture)}", colorBar)
            ;
    }
    
    private int MapValue(double value, double inMin, double inMax, double outMin, double outMax)
    {
        return (int)Math.Round((value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin);
    }

    private string CaptureImage(FswebcamParameters parameters)
    {
        Process process = new()
        {
            StartInfo = new ProcessStartInfo
            {
                FileName = "fswebcam",
                Arguments = parameters.ToString(),
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            }
        };
        
        Logger.LogDebug("Start process: fswebcam for {video}", parameters.Device);
        Logger.LogTrace("fswebcam {parameter}", parameters.ToString());

        process.Start();
        if (!process.WaitForExit(TimeSpan.FromSeconds(10)))
        {
            process.Kill();
        }

        var errorStream = process.StandardError.ReadToEnd();
        var standardStream = process.StandardOutput.ReadToEnd();
        var stream = standardStream + " " + errorStream;
        
        Logger.LogDebug("Image capture output for {device} : {stream}", parameters.Device, stream);
        
        if (process.ExitCode == 0)
        {
            if (stream.Contains("No frames captured"))
            {
                throw new WebcamException(stream);
            }
            
            Logger.LogInformation("Image captured successfully for device {device}", parameters.Device);

            return parameters.SaveFile;
        }

        Logger.LogError("Error capturing image for device {device}. Exit code: {exitCode}", parameters.Device, process.ExitCode);
            
        throw new WebcamException(stream);
    }
    
    private void ImageMagick(ImageMagickParameters parameters)
    {
        Process process = new()
        {
            StartInfo = new ProcessStartInfo
            {
                FileName = "convert",
                Arguments = parameters.ToString(),
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            }
        };
        
        Logger.LogDebug("Start process: convert");
        Logger.LogTrace("convert {command}", parameters.ToString());

        process.Start();
        if (!process.WaitForExit(TimeSpan.FromMinutes(1)))
        {
            process.Kill();
        }
        
        var errorStream = process.StandardError.ReadToEnd();
        var standardStream = process.StandardOutput.ReadToEnd();
        var streamString = standardStream + " " + errorStream;
        
        if (!string.IsNullOrWhiteSpace(errorStream))
        {
            throw new WebcamException($"ImageMagick error with exit code {process.ExitCode}.\n\nCommand:\n\nconvert {parameters}.\n\nOutput:\n\n{streamString}");
        }
    }
}