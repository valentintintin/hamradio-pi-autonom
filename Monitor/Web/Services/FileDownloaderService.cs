using HttpRequestException = Web.Exceptions.HttpRequestException;

namespace Web.Services;

public class FileDownloaderService
{
    public async Task DownloadFileAsync(string? fileUrl, string filePath)
    {
        using HttpClient client = new();
        HttpResponseMessage response = await client.GetAsync(fileUrl);
        if (response.IsSuccessStatusCode)
        {
            await using Stream stream = await response.Content.ReadAsStreamAsync();
            await using FileStream fileStream = new(filePath, FileMode.Create);
            await stream.CopyToAsync(fileStream);
        }
        else
        {
            throw new HttpRequestException(response);
        }
    }
}