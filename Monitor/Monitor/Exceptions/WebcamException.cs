namespace Monitor.Exceptions;

public class WebcamException(string error, Exception? innerException = null) : Exception(error, innerException);