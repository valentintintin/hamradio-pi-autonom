namespace Monitor.Exceptions;

public class WebcamException : Exception
{
    public WebcamException(string error, Exception? innerException = null) : base(error, innerException) {}
}