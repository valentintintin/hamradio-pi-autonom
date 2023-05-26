namespace Monitor.Exceptions;

public class MessageParseException : Exception
{
    public MessageParseException(string input) : base($"Issue during parse of {input}") {}
    
    public MessageParseException(Exception e, string input) : base($"Issue during parse of {input}", e) {}
}