using System.Text.Json.Serialization;

namespace Monitor.Models.SerialMessages;

public class McuSystemData : Message
{
    [JsonPropertyName("state")]
    public required string State { get; set; }

    [JsonPropertyName("boxOpened")]
    public bool BoxOpened { get; set; }

    public bool IsAlert => State == "alert";

    public override string ToString()
    {
        return $"{base.ToString()} is {State}. Box is {(BoxOpened ? "opened" : "closed")}";
    }
}