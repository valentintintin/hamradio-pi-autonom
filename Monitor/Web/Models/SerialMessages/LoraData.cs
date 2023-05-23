using System.Text.Json.Serialization;

namespace Web.Models.SerialMessages;

public class LoraData : Message
{
    [JsonPropertyName("state")]
    public string State { get; set; }

    [JsonPropertyName("payload")]
    public string Payload { get; set; }

    public bool IsTx => State == "tx";

    public override string ToString()
    {
        return $"{base.ToString()} {State} with payload {Payload}";
    }
}