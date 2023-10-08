using System.Diagnostics;
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
    private readonly string _storagePath;
    private readonly string? _message;
    private readonly Font _fontTitle, _fontInfo, _fontFooter;
    private readonly ImageEncoder _imageEncoder;
    private const int WidthData = 500;
    private const int MarginData = 10;
    private readonly int _widthMaxProgressBar;
    private readonly List<FswebcamParameters> _fswebcamParameters;
    
    public CameraService(ILogger<CameraService> logger, IConfiguration configuration) : base(logger)
    {
        IConfigurationSection configurationSection = configuration.GetSection("Cameras");
        _message = configurationSection.GetValue<string?>("Message");
        _fswebcamParameters = configurationSection.GetSection("Devices").Get<List<FswebcamParameters>>() ?? new List<FswebcamParameters>();
        
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

        _widthMaxProgressBar = WidthData - MarginData * 2;
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
        
        List<Image> imagesCamera = new();

        foreach (Task<string> file in CaptureAllCameras())
        {
            try
            {
                imagesCamera.Add(await Image.LoadAsync(await file));
            }
            catch (Exception e)
            {
                Logger.LogError(e, "Error during reading memorystream of a camera capture");
            }
        }

        Logger.LogTrace("We have {count} cameras", imagesCamera.Count);

        int width = 0;
        int height = 480;

        if (imagesCamera.Any())
        {
            width = imagesCamera.Sum(i => i.Width);
            height = imagesCamera.Max(i => i.Height);
        }
        
        using Image resultImage = new Image<Rgb24>(width + WidthData, height);
        using Image dataImage = new Image<Rgb24>(WidthData, height);

        dataImage.Mutate(ctx =>
        {
            ctx.DrawText(
                $"{DateTime.UtcNow:G} UTC",
                _fontTitle,
                Color.White,
                new PointF(MarginData + 100, 10));

            DrawProgressBarwithInfo(ctx, 0, "Tension batterie", "mV", EntitiesManagerService.Entities.BatteryVoltage.Value, 11000, 13500, Color.Red, Color.Black);
            DrawProgressBarwithInfo(ctx, 1, "Courant batterie", "mA", EntitiesManagerService.Entities.BatteryCurrent.Value, 0, 1000, Color.Yellow, Color.Black);
            DrawProgressBarwithInfo(ctx, 2, "Tension panneau", "mV", EntitiesManagerService.Entities.SolarVoltage.Value, 0, 25000, Color.Red, Color.Black);
            DrawProgressBarwithInfo(ctx, 3, "Courant panneau", "mA", EntitiesManagerService.Entities.SolarCurrent.Value, 0, 1000, Color.Yellow, Color.Black);
            // DrawProgressBarwithInfo(ctx, 4, "Température", "°C", EntitiesManagerService.Entities.WeatherTemperature.Value, -10, 40, Color.LightGreen, Color.Black);
            // DrawProgressBarwithInfo(ctx, 5, "Humidité", "%", EntitiesManagerService.Entities.WeatherHumidity.Value, 0, 100, Color.LightBlue, Color.Black);

            if (!string.IsNullOrWhiteSpace(_message))
            {
                ctx.DrawText(
                    _message,
                    _fontFooter,
                    Color.White,
                    new PointF(WidthData - MarginData - 100, height - 30));
            }
        });
        
        resultImage.Mutate(ctx =>
        {
            int x = 0;

            foreach (Image image in imagesCamera)
            {
                ctx.DrawImage(image, new Point(x, 0), 1);
                x += image.Width;
            }

            ctx.DrawImage(dataImage, new Point(width, 0), 1);
        });
        
        MemoryStream stream = new();
        await resultImage.SaveAsync(stream, _imageEncoder);
        stream.Seek(0, SeekOrigin.Begin);

        if (save)
        {
            Directory.CreateDirectory($"{_storagePath}/{DateTime.UtcNow:yyyy-MM-dd-HH-mm-ss}");
            
            string filePath = $"{_storagePath}/{DateTime.UtcNow:yyyy-MM-dd-HH-mm-ss}/{DateTime.UtcNow:yyyy-MM-dd-HH-mm-ss}-{Random.Shared.NextInt64()}.webp";
            string lastPath = $"{_storagePath}/last.webp";
            
            Logger.LogTrace("Save final image to {path}", filePath);

            await resultImage.SaveAsync(filePath, _imageEncoder);
            
            File.Delete(lastPath);
            File.CreateSymbolicLink(lastPath, filePath);
        }
        
        Logger.LogInformation("Create final image OK");

        return stream;
    }

    private List<Task<string>> CaptureAllCameras()
    {
        Logger.LogInformation("Will capture images from cameras : {cameras}", _fswebcamParameters.Select(j => j.Device).JoinString());

        return _fswebcamParameters.Select(async parameters =>
            {
                string cameraName = parameters.Device.Replace("/dev/", string.Empty);

                Logger.LogTrace("Capture image from {camera}", cameraName);

                try
                {
                    parameters.SaveFile = $"/{_storagePath}/{cameraName}.jpg";
                    
                    return await CaptureImage(parameters);
                }
                catch (Exception e)
                {
                    Logger.LogError(e, "Capture image from {camera} KO", cameraName);
                }

                return null;
            })
            .Where(c => c != null)
            .Cast<Task<string>>()
            .ToList()!;
    }

    private void DrawProgressBarwithInfo(IImageProcessingContext ctx, int indexValue, string label, string unit, double value, double min, double max, Color colorBar, Color colorText)
    {
        int y = indexValue * 50 + 50;
        
        ctx.Fill(Color.LightGray, new Rectangle(MarginData, y, _widthMaxProgressBar, 24));
        ctx.Fill(colorBar, new Rectangle(MarginData, y, 
            MapValue(value, min, max, 0, _widthMaxProgressBar),
            24));
        ctx.DrawText(
            $"{label}: {value}{unit}",
            _fontInfo,
            colorText,
            new PointF(MarginData + 50, y));
    }
    
    private int MapValue(double value, double inMin, double inMax, double outMin, double outMax)
    {
        return (int)Math.Round((value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin);
    }

    private async Task<string> CaptureImage(FswebcamParameters parameters)
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
        await process.WaitForExitAsync();

        string errorStream = await process.StandardError.ReadToEndAsync();
        string standardStream = await process.StandardOutput.ReadToEndAsync();
        string stream = standardStream + " " + errorStream;
        
        Logger.LogTrace("Image capture output for {device} : {stream}", parameters.Device, stream);
        
        if (process.ExitCode == 0)
        {
            if (errorStream.Contains("No frames captured") || standardStream.Contains("No frames captured"))
            {
                throw new WebcamException(stream);
            }
            
            Logger.LogInformation("Image captured successfully for device {device}", parameters.Device);

            return parameters.SaveFile;
        }

        Logger.LogError("Error capturing image for device {device}. Exit code: {exitCode}", parameters.Device, process.ExitCode);
            
        throw new WebcamException(stream);
    }
}