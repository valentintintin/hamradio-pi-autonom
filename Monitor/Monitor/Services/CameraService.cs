using FlashCap;
using Monitor.Extensions;
using SixLabors.Fonts;
using SixLabors.ImageSharp.Drawing.Processing;
using SixLabors.ImageSharp.Formats;
using SixLabors.ImageSharp.Formats.Jpeg;
using SixLabors.ImageSharp.Formats.Png;
using SixLabors.ImageSharp.Formats.Webp;

namespace Monitor.Services;

public class CameraService : AService
{
    private readonly string _storagePath;
    private readonly string? _message;
    private readonly Font _fontTitle, _fontInfo, _fontFooter;
    private readonly ImageEncoder _imageEncoder;
    private readonly CaptureDevices _devices;
    private const int WidthData = 500;
    private const int MarginData = 10;
    private readonly int _widthMaxProgressBar;
    
    public CameraService(ILogger<CameraService> logger, IConfiguration configuration) : base(logger)
    {
        IConfigurationSection configurationSection = configuration.GetSection("Cameras");
        _message = configurationSection.GetValue<string?>("Message");
        
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
        _devices = new CaptureDevices();

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

        foreach (Task<MemoryStream> memoryStream in CaptureAllCameras())
        {
            try
            {
                imagesCamera.Add(await Image.LoadAsync(await memoryStream));
            }
            catch (Exception e)
            {
                Logger.LogError(e, "Error during reading memorystream of a camera capture");
            }
        }

        Logger.LogDebug("We have {count} cameras", imagesCamera.Count);

        int width = 0;
        int height = 0;

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

            DrawProgressBarwithInfo(ctx, 0, "Tension batterie", "mV", EntitiesManagerService.Entities.BatteryVoltage.Value, 11000, 13000, Color.Red, Color.Black);
            DrawProgressBarwithInfo(ctx, 1, "Courant batterie", "mA", EntitiesManagerService.Entities.BatteryCurrent.Value, 0, 2000, Color.Yellow, Color.Black);
            DrawProgressBarwithInfo(ctx, 2, "Tension panneau", "mV", EntitiesManagerService.Entities.SolarVoltage.Value, 0, 25000, Color.Red, Color.Black);
            DrawProgressBarwithInfo(ctx, 3, "Courant panneau", "mA", EntitiesManagerService.Entities.SolarCurrent.Value, 0, 2000, Color.Yellow, Color.Black);
            DrawProgressBarwithInfo(ctx, 4, "Température", "°C", EntitiesManagerService.Entities.WeatherTemperature.Value, -10, 40, Color.LightGreen, Color.Black);
            DrawProgressBarwithInfo(ctx, 5, "Humidité", "%", EntitiesManagerService.Entities.WeatherHumidity.Value, 0, 100, Color.LightBlue, Color.Black);

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
            string filePath = $"{_storagePath}/{DateTime.UtcNow:yyyy-MM-dd--HH-mm-ss}-{Random.Shared.NextInt64()}.webp";
            string lastPath = $"{_storagePath}/last.webp";
            
            Logger.LogTrace("Save final image to {path}", filePath);

            await resultImage.SaveAsync(filePath, _imageEncoder);
            
            File.Delete(lastPath);
            File.CreateSymbolicLink(lastPath, filePath);
        }
        
        Logger.LogInformation("Create final image OK");

        return stream;
    }

    private List<Task<MemoryStream>> CaptureAllCameras()
    {
        List<CaptureDeviceDescriptor> cameras = _devices.EnumerateDescriptors().ToList();
        
        Logger.LogInformation("Captures images from {cameras}", cameras.Select(j => (string) j.Identity).JoinString());

        return cameras.Select(async camera =>
            {
                string cameraName = ((string)camera.Identity).Replace("/dev/", string.Empty);

                Logger.LogTrace("Capture image from {camera}", cameraName);

                try
                {
                    return new MemoryStream(
                        await camera.TakeOneShotAsync(
                            camera.Characteristics.FirstOrDefault() ?? new VideoCharacteristics(PixelFormats.JPEG, 640, 480, 1)
                        )
                    );
                }
                catch (Exception e)
                {
                    Logger.LogError(e, "Capture image from {camera} KO", cameraName);
                }

                return null;
            })
            .Where(c => c != null)
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
}