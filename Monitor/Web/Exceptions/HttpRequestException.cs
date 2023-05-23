namespace Web.Exceptions;

public class HttpRequestException : Exception
{
    public HttpRequestException(HttpResponseMessage responseMessage) : base($"Error HTTP : {responseMessage.StatusCode}") {}
}