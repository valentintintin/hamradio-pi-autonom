using System.Text.Json.Serialization;

namespace Monitor.Models.SerialMessages;

public class GpioData : Message
{
    [JsonPropertyName("state")]
    public int State { get; set; }

    [JsonPropertyName("name")]
    public string Name { get; set; }
    
    [JsonPropertyName("pin")]
    public int Pin { get; set; }

    public bool Enabled => State > 0;

    public override string ToString()
    {
        return $"{base.ToString()} {Name} is {State}";
    }
}