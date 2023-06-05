namespace Monitor.Extensions;

public static class HttpResponseMessageExtensions
{
    public static async Task Log(this HttpResponseMessage message, ILogger? logger = null)
    {
        if (!message.IsSuccessStatusCode)
        {
            logger?.LogWarning("Requête KO : {url} {statusCode} => {statusMessage}",
                message.RequestMessage?.RequestUri, message.StatusCode, await message.Content.ReadAsStringAsync());
        }
    }
}