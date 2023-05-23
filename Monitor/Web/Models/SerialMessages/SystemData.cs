using System.Text.Json.Serialization;

namespace Web.Models.SerialMessages;

public class SystemData : Message
{
    [JsonPropertyName("state")]
    public string State { get; set; }

    [JsonPropertyName("boxOpened")]
    public bool BoxOpened { get; set; }

    public bool IsAlert => State == "alert";

    public override string ToString()
    {
        return $"{base.ToString()} is {State}. Box is {(BoxOpened ? "opened" : "closed")}";
    }
}