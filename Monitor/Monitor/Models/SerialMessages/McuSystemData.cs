using System.Text.Json.Serialization;

namespace Monitor.Models.SerialMessages;

public class McuSystemData : Message
{
    [JsonPropertyName("state")]
    public required string State { get; set; }

    [JsonPropertyName("boxOpened")]
    public bool BoxOpened { get; set; }

    [JsonPropertyName("watchdogSafety")]
    public bool WatchdogSafetyEnabled { get; set; }

    [JsonPropertyName("aprsDigipeater")]
    public bool AprsDigipeaterEnabled { get; set; }

    [JsonPropertyName("aprsTelemetry")]
    public bool AprsTelemetryEnabled { get; set; }

    [JsonPropertyName("aprsPosition")]
    public bool AprsPositionEnabled { get; set; }

    [JsonPropertyName("sleep")]
    public bool Sleep { get; set; }

    [JsonPropertyName("temperatureRtc")]
    public float TemperatureRtc { get; set; }

    [JsonIgnore]
    public bool IsAlert => State == "alert";
}