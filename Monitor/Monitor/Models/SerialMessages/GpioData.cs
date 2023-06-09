using System.Text.Json.Serialization;

namespace Monitor.Models.SerialMessages;

public class GpioData : Message
{
    [JsonPropertyName("wifi")]
    public bool Wifi { get; set; }
    
    [JsonPropertyName("npr")]
    public bool Npr { get; set; }
    
    [JsonPropertyName("ldr")]
    public int Ldr { get; set; }
}