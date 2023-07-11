using System.Text.Json.Serialization;

namespace Monitor.Models.SerialMessages;

public class Message
{
    [JsonIgnore]
    public DateTime ReceivedAt { get; } = DateTime.UtcNow;
    
    [JsonPropertyName("type")]
    public required string Type { get; set; }
}