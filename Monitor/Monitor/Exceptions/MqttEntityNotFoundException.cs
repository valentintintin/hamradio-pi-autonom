namespace Monitor.Exceptions;

public class MqttEntityNotFoundException : Exception
{
    public MqttEntityNotFoundException(string topic) : base($"MqttEntity topic {topic} not found")
    {
    }
}