namespace Monitor.Exceptions;

public class MqttEntityNotFoundException(string topic) : Exception($"MqttEntity topic {topic} not found");