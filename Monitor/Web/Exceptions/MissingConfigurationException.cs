namespace Web.Exceptions;

public class MissingConfigurationException : Exception
{
    public MissingConfigurationException(string key) : base($"Missing key in configuration : {key}")
    {
    }
}