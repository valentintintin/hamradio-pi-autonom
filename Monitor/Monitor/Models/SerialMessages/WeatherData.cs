using System.Text.Json.Serialization;

namespace Monitor.Models.SerialMessages;

public class WeatherData : Message
{
    [JsonPropertyName("temperature")]
    public float Temperature { get; set; }

    [JsonPropertyName("humidity")]
    public float Humidity { get; set; }

    [JsonPropertyName("pressure")]
    public float Pressure { get; set; }
}