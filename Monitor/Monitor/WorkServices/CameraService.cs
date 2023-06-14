using FlashCap;
using Monitor.Extensions;
using NetDaemon.HassModel;
using NetDaemon.HassModel.Entities;
using SixLabors.Fonts;
using SixLabors.ImageSharp.Drawing.Processing;
using SixLabors.ImageSharp.Formats.Jpeg;

namespace Monitor.WorkServices;

public class CameraService : AService
{
    private readonly string _storagePath;
    private readonly Font _fontTitle, _fontInfo, _fontFooter;
    private readonly JpegEncoder _jpegEncoder;
    private readonly CaptureDevices _devices;
    
    private readonly int _widthData = 500;
    private readonly int _marginData = 10;
    private readonly int _widthMaxProgressBar;
    
    public CameraService(ILogger<CameraService> logger, IConfiguration configuration) : base(logger)
    {
        _storagePath = Path.Combine(
            configuration.GetValueOrThrow<string>("StoragePath"), 
            configuration.GetSection("Cameras").GetValueOrThrow<string>("Path")
        );

        Directory.CreateDirectory($"{_storagePath}");
        Directory.CreateDirectory($"{_storagePath}/final");
        
        Logger.LogInformation("Cameras capture are in {storagePath}", _storagePath);
        
        FontCollection collection = new();
        FontFamily family = collection.Add("Arial.ttf");
        _fontTitle = family.CreateFont(30, FontStyle.Bold);
        _fontInfo = family.CreateFont(20, FontStyle.Regular);
        _fontFooter = family.CreateFont(15, FontStyle.Regular);
        _jpegEncoder = new JpegEncoder();
        _devices = new CaptureDevices();

        _widthMaxProgressBar = _widthData - _marginData * 2;
    }

    public string? GetFinalLast()
    {
        string path = $"{_storagePath}/final/last.jpg";
        return File.Exists(path) ? path : null;
    }

    public async Task CaptureAllCameras()
    {
        List<CaptureDeviceDescriptor> cameras = _devices.EnumerateDescriptors().ToList();
        
        Logger.LogInformation("Captures images from {cameras}", cameras.Select(j => (string) j.Identity).JoinString());
        
        foreach (CaptureDeviceDescriptor camera in cameras)
        {
            string cameraName = ((string) camera.Identity).Replace("/dev/", string.Empty);
            string path = $"{_storagePath}/{cameraName}/{DateTime.UtcNow:yyyy-MM-dd--HH-mm-ss}.jpg";
            string lastPath = $"{_storagePath}/{cameraName}/last.jpg";

            Logger.LogTrace("Capture image from {camera} to {path}", cameraName, path);

            try
            {
                byte[] imageData = await camera.TakeOneShotAsync(camera.Characteristics.FirstOrDefault() ??
                                    new VideoCharacteristics(PixelFormats.JPEG, 640, 480, 1));

                Directory.CreateDirectory($"{_storagePath}/{cameraName}");

                await File.WriteAllBytesAsync(path, imageData);
                File.Delete(lastPath);
                File.CreateSymbolicLink(lastPath, path);
            }
            catch (Exception e)
            {
                Logger.LogError(e, "Capture image from {camera} KO", cameraName);
            }
        }
    }

    public MemoryStream CreateFinalImageFromLasts(bool save = true)
    {
        Logger.LogInformation("Create final image");
        
        List<Image> imagesCamera = GetAllCameraLast()
            .Select(Image.Load)
            .ToList();

        Logger.LogDebug("We have {count} cameras", imagesCamera.Count);

        int width = imagesCamera.Sum(i => i.Width);
        int height = imagesCamera.Max(i => i.Height);
        
        using Image resultImage = new Image<Rgb24>(width + _widthData, height);
        using Image dataImage = new Image<Rgb24>(_widthData, height);

        dataImage.Mutate(ctx =>
        {
            ctx.DrawText(
                DateTime.Now.ToString("G"),
                _fontTitle,
                Color.White,
                new PointF(_marginData + 100, 10));

            DrawProgressBarwithInfo(ctx, 0, "Battery voltage", "mV", MonitorService.State.Mppt.BatteryVoltage, 11000, 13000, Color.Red, Color.Black);
            DrawProgressBarwithInfo(ctx, 1, "Battery current", "mA", MonitorService.State.Mppt.BatteryCurrent, 0, 2000, Color.Yellow, Color.Black);
            DrawProgressBarwithInfo(ctx, 2, "Solar voltage", "mV", MonitorService.State.Mppt.SolarVoltage, 0, 25000, Color.Red, Color.Black);
            DrawProgressBarwithInfo(ctx, 3, "Solar current", "mA", MonitorService.State.Mppt.SolarCurrent, 0, 2000, Color.Yellow, Color.Black);
            DrawProgressBarwithInfo(ctx, 4, "Temperature", "°C", MonitorService.State.Weather.Temperature, -10, 40, Color.LightGreen, Color.Black);
            DrawProgressBarwithInfo(ctx, 5, "Humidity", "%", MonitorService.State.Weather.Humidity, 0, 100, Color.LightBlue, Color.Black);
            
            ctx.DrawText(
                "Valentin F4HVV",
                _fontFooter,
                Color.White,
                new PointF(_widthData - _marginData - 100, height - 30)); 
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
        resultImage.Save(stream, _jpegEncoder);
        stream.Seek(0, SeekOrigin.Begin);

        if (save)
        {
            string filePath = $"{_storagePath}/final/{DateTime.UtcNow:yyyy-MM-dd--HH-mm-ss}.jpg";
            string lastPath = $"{_storagePath}/final/last.jpg";
            
            Logger.LogTrace("Save final image to {path}", filePath);

            resultImage.Save(filePath, _jpegEncoder);
            
            File.Delete(lastPath);
            File.CreateSymbolicLink(lastPath, filePath);
        }
        
        Logger.LogInformation("Create final image OK");

        return stream;
    }

    private List<string> GetAllCameraLast()
    {
        return Directory.GetDirectories(_storagePath)
            .Where(d => !d.EndsWith("final"))
            .Select(d => $"{d}/last.jpg")
            .Where(File.Exists)
            .ToList();
    }

    private void DrawProgressBarwithInfo(IImageProcessingContext ctx, int indexValue, string label, string unit, double value, double min, double max, Color colorBar, Color colorText)
    {
        int y = indexValue * 50 + 50;
        
        ctx.Fill(Color.LightGray, new Rectangle(_marginData, y, _widthMaxProgressBar, 24));
        ctx.Fill(colorBar, new Rectangle(_marginData, y, 
            MapValue(value, min, max, 0, _widthMaxProgressBar),
            24));
        ctx.DrawText(
            $"{label}: {value}{unit}",
            _fontInfo,
            colorText,
            new PointF(_marginData + 50, y));
    }
    
    private int MapValue(double value, double inMin, double inMax, double outMin, double outMax)
    {
        return (int)Math.Round((value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin);
    }
}