using System.Text.Json.Serialization;

namespace Monitor.Models.SerialMessages;

public class Message
{
    [JsonPropertyName("type")]
    public required string Type { get; set; }

    public override string ToString()
    {
        return $"Message: {Type}";
    }
}