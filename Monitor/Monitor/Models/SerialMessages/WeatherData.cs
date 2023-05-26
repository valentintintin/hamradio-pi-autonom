using System.Text.Json.Serialization;

namespace Monitor.Models.SerialMessages;

public class WeatherData : Message
{
    [JsonPropertyName("temperature")]
    public double Temperature { get; set; }

    [JsonPropertyName("humidity")]
    public int Humidity { get; set; }

    public override string ToString()
    {
        return $"{base.ToString()} is {Temperature}°C with {Humidity}% humidity";
    }
}