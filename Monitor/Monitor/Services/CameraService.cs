using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using Monitor.Exceptions;
using Monitor.Extensions;
using Monitor.Models;
using SixLabors.Fonts;
using SixLabors.ImageSharp.Drawing.Processing;
using SixLabors.ImageSharp.Formats;
using SixLabors.ImageSharp.Formats.Webp;

namespace Monitor.Services;

public class CameraService : AService
{
    private bool _isRunning;

    private readonly string _storagePath;
    private readonly string? _message;
    private readonly Font _fontTitle, _fontInfo, _fontFooter;
    private readonly ImageEncoder _imageEncoder;
    private readonly List<FswebcamParameters> _fswebcamParameters;

    private const int FrameToTakeDuringNight = 25;
    private const int WidthData = 700;
    private const int LegendSize = 70;
    private const int LegendMargin = 15;
    private readonly int _widthMaxProgressBar;

    public CameraService(ILogger<CameraService> logger, IConfiguration configuration) : base(logger)
    {
        IConfigurationSection configurationSection = configuration.GetSection("Cameras");
        _message = configurationSection.GetValue<string?>("Message");
        _fswebcamParameters = configurationSection.GetSection("Devices").Get<List<FswebcamParameters>>()?.Distinct().ToList() ?? new List<FswebcamParameters>();
        
        _storagePath = Path.Combine(
            configuration.GetValueOrThrow<string>("StoragePath"), 
            configurationSection.GetValueOrThrow<string>("Path")
        );
        Directory.CreateDirectory($"{_storagePath}");

        FontCollection collection = new();
        FontFamily family = collection.Add("Arial.ttf");
        _fontTitle = family.CreateFont(30, FontStyle.Bold);
        _fontInfo = family.CreateFont(20, FontStyle.Regular);
        _fontFooter = family.CreateFont(15, FontStyle.Regular);
        _imageEncoder = new WebpEncoder
        {
            Quality = 80,
            SkipMetadata = true,
            Method = WebpEncodingMethod.Fastest,
        };

        _widthMaxProgressBar = WidthData - (LegendSize + LegendMargin) * 2;
    }

    public string? GetFinalLast()
    {
        string path = $"{_storagePath}/last.webp";

        if (!File.Exists(path))
        {
            Logger.LogWarning("Last final image does not exist at {path}", path);
            
            return null;
        }
        
        FileSystemInfo? resolveLinkTarget = File.ResolveLinkTarget(path, true);

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
        
        List<Image> imagesCamera = new();

        foreach (string file in CaptureAllCameras())
        {
            Logger.LogTrace("Load photo {file}", file);
            
            imagesCamera.Add(await Image.LoadAsync(file));
        }

        Logger.LogTrace("We have {count} photos", imagesCamera.Count);

        int width = 0;
        int height = 480;

        if (imagesCamera.Any())
        {
            width = imagesCamera.Sum(i => i.Width);
            height = imagesCamera.Max(i => i.Height);
        }
        
        using Image resultImage = new Image<Rgb24>(width + WidthData, height);
        using Image dataImage = new Image<Rgb24>(WidthData, height);

        DateTime now = DateTime.UtcNow;
        
        dataImage.Mutate(ctx =>
        {
            Logger.LogTrace("Draw title on image");

            ctx.DrawText(
                $"{now.ToFrench():G}",
                _fontTitle,
                Color.White,
                new PointF(LegendMargin + LegendSize, 10));
            
            DrawProgressBarwithInfo(ctx, 0, "Tension batterie", "mV", EntitiesManagerService.Entities.BatteryVoltage.Value, 11000, 13500, Color.IndianRed, Color.Black);
            DrawProgressBarwithInfo(ctx, 1, "Courant batterie", "mA", EntitiesManagerService.Entities.BatteryCurrent.Value, 0, 1000, Color.Cyan, Color.Black);
            DrawProgressBarwithInfo(ctx, 2, "Tension panneau", "mV", EntitiesManagerService.Entities.SolarVoltage.Value, 0, 25000, Color.Yellow, Color.Black);
            DrawProgressBarwithInfo(ctx, 3, "Courant panneau", "mA", EntitiesManagerService.Entities.SolarCurrent.Value, 0, 3000, Color.LightGreen, Color.Black);
            DrawProgressBarwithInfo(ctx, 4, "Température", "°C", EntitiesManagerService.Entities.WeatherTemperature.Value, -10, 40, Color.DarkOrange, Color.Black);
            DrawProgressBarwithInfo(ctx, 5, "Humidité", "%", EntitiesManagerService.Entities.WeatherHumidity.Value, 0, 100, Color.LightBlue, Color.Black);
            DrawProgressBarwithInfo(ctx, 6, "Pression", "hPa", EntitiesManagerService.Entities.WeatherPressure.Value, 800, 1200, Color.LightPink, Color.Black);

            if (!string.IsNullOrWhiteSpace(_message))
            {
                Logger.LogTrace("Draw footer on image");

                ctx.DrawText(
                    _message,
                    _fontFooter,
                    Color.White,
                    new PointF(LegendMargin, height - 30));
            }
        });
        
        resultImage.Mutate(ctx =>
        {
            int x = 0;

            foreach (Image image in imagesCamera)
            {
                Logger.LogTrace("Draw image at {x} on image", x);

                ctx.DrawImage(image, new Point(x, 0), 1);
                x += image.Width;
            }
            
            Logger.LogTrace("Draw date image on image");

            ctx.DrawImage(dataImage, new Point(width, 0), 1);
        });
        
        Logger.LogTrace("Saving image to stream");
        
        MemoryStream stream = new();
        await resultImage.SaveAsync(stream, _imageEncoder);
        stream.Seek(0, SeekOrigin.Begin);

        if (save)
        {
            Directory.CreateDirectory($"{_storagePath}/{now:yyyy-MM-dd}");
            
            string filePath = $"{_storagePath}/{now:yyyy-MM-dd}/{now:yyyy-MM-dd-HH-mm-ss}-{Random.Shared.NextInt64()}.webp";
            string lastPath = $"{_storagePath}/last.webp";
            
            Logger.LogTrace("Save final image to {path}", filePath);

            await resultImage.SaveAsync(filePath, _imageEncoder);
            
            File.Delete(lastPath);
            File.CreateSymbolicLink(lastPath, filePath);
        }
        
        Logger.LogInformation("Create final image OK");
        
        _isRunning = false;
        
        return stream;
    }

    private List<string> CaptureAllCameras()
    {
        Logger.LogInformation("Will capture images from cameras : {cameras}", _fswebcamParameters.Select(j => j.Device).JoinString());

        return _fswebcamParameters.Select(parameters =>
        {
            string cameraName = parameters.Device.Replace("/dev/v4l/by-id/", string.Empty);

            Logger.LogTrace("Capture image from {camera}", cameraName);

            parameters.Frames = MonitorService.State.Mppt.Night ? FrameToTakeDuringNight : null;
            parameters.SaveFile = $"/{_storagePath}/{cameraName}.jpg";
            
            return CaptureImage(parameters);
        }).ToList()!;
    }
    
    private void DrawProgressBarwithInfo(IImageProcessingContext ctx, int indexValue, string label, string unit, double value, double min, double max, Color colorBar, Color colorText)
    {
        Logger.LogTrace("Draw bar on image for {label}", label);

        int y = indexValue * 50 + 60;
        
        ctx.Fill(Color.LightGray, new Rectangle(LegendMargin + LegendSize, y, _widthMaxProgressBar, 24));
        ctx.Fill(colorBar, new Rectangle(LegendMargin + LegendSize, y, 
            (int) Math.Round((double) MapValue(value, min, max, 0, _widthMaxProgressBar), 2),
            24));
        ctx.DrawText(
            $"{label}: {value} {unit}",
            _fontInfo,
            colorText,
            new PointF(LegendMargin + LegendSize + 50, y + 2));
        ctx.DrawText(
            $"{min.ToString(CultureInfo.CurrentCulture)}",
            _fontInfo,
            Color.White,
            new PointF(LegendMargin, y + 2));
        ctx.DrawText(
            $"{max.ToString(CultureInfo.CurrentCulture)}",
            _fontInfo,
            Color.White,
            new PointF(LegendMargin + LegendSize + _widthMaxProgressBar + 5, y + 2));
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

        process.Start();
        process.WaitForExit();

        string errorStream = process.StandardError.ReadToEnd();
        string standardStream = process.StandardOutput.ReadToEnd();
        string stream = standardStream + " " + errorStream;
        
        Logger.LogTrace("Image capture output for {device} : {stream}", parameters.Device, stream);
        
        if (process.ExitCode == 0)
        {
            if (errorStream.Length > 0 || standardStream.Contains("No frames captured"))
            {
                // throw new WebcamException(stream); // TODO uncomment
            }
            
            Logger.LogInformation("Image captured successfully for device {device}", parameters.Device);

            return parameters.SaveFile;
        }

        Logger.LogError("Error capturing image for device {device}. Exit code: {exitCode}", parameters.Device, process.ExitCode);
            
        throw new WebcamException(stream);
    }
}