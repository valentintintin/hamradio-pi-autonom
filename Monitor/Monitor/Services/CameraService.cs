namespace Monitor.Services;

public class CameraService : AService
{
    private readonly string _storagePath = "storage/camera";
    
    public CameraService(ILogger<CameraService> logger) : base(logger)
    {
    }

    public string[] GetAll()
    {
        return Directory.GetFiles(_storagePath, "*.jpg");
    }

    public string? GetLast()
    {
        return GetAll().LastOrDefault();
    }

    public async Task Capture(string webcamUrl)
    {
        string filePath = $"{_storagePath}/{DateTime.UtcNow:u}.jpg";
            
        try
        {
            await DownloadFileAsync(webcamUrl, filePath);
            Logger.LogInformation("Get webcam snapshot to {filePath} OK", filePath);
        }
        catch (Exception e)
        {
            Logger.LogError("Get webcam snapshot to {filePath} KO", filePath);
        }
    }
    
    private async Task DownloadFileAsync(string? fileUrl, string filePath)
    {
        using HttpClient client = new();
        
        Logger.LogTrace("Get {fileUrl} to {filePath}", fileUrl, filePath);
        
        HttpResponseMessage response = await client.GetAsync(fileUrl);
        
        if (response.IsSuccessStatusCode)
        {
            await using Stream stream = await response.Content.ReadAsStreamAsync();
            await using FileStream fileStream = new(filePath, FileMode.Create);
            await stream.CopyToAsync(fileStream);
            
            Logger.LogInformation("Get {fileUrl} to {filePath}. Size {size} OK", fileUrl, filePath, fileStream.Length / 1024 / 1024);
        }
        else
        {
            Logger.LogWarning("Get {fileUrl} to {filePath} KO. Status code {statusCode} : {content}", fileUrl, filePath, response.StatusCode, await response.Content.ReadAsStringAsync());
            
            throw new HttpRequestException();
        }
    }
}