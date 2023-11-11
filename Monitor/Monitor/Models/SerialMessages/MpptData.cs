using System.Text.Json.Serialization;

namespace Monitor.Models.SerialMessages;

public class MpptData : Message
{
    [JsonPropertyName("batteryVoltage")]
    public int BatteryVoltage { get; set; }

    [JsonPropertyName("batteryCurrent")]
    public int BatteryCurrent { get; set; }

    [JsonPropertyName("solarVoltage")]
    public int SolarVoltage { get; set; }

    [JsonPropertyName("solarCurrent")]
    public int SolarCurrent { get; set; }

    [JsonPropertyName("currentCharge")]
    public int CurrentCharge { get; set; }

    [JsonPropertyName("status")]
    public int Status { get; set; }

    [JsonPropertyName("statusString")]
    public string? StatusString { get; set; }

    [JsonPropertyName("night")]
    public bool Night { get; set; }

    [JsonPropertyName("alert")]
    public bool Alert { get; set; }

    [JsonPropertyName("watchdogEnabled")]
    public bool WatchdogEnabled { get; set; }

    [JsonPropertyName("watchdogPowerOffTime")]
    public int WatchdogPowerOffTime { get; set; }

    [JsonPropertyName("watchdogCounter")]
    public int WatchdogCounter { get; set; }

    [JsonPropertyName("powerEnabled")]
    public bool PowerEnabled { get; set; }

    [JsonPropertyName("powerOffVoltage")]
    public int PowerOffVoltage { get; set; }

    [JsonPropertyName("powerOnVoltage")]
    public int PowerOnVoltage { get; set; }
    
    [JsonPropertyName("temperature")]
    public float Temperature { get; set; }

    [JsonIgnore]
    public TimeSpan WatchdogCounterTimeSpan => TimeSpan.FromSeconds(WatchdogCounter);
    [JsonIgnore]
    public TimeSpan WatchdogPowerOffTimeSpan => TimeSpan.FromSeconds(WatchdogPowerOffTime);
    [JsonIgnore]
    public DateTime WatchdogPowerOffDateTime => DateTime.UtcNow.Add(WatchdogPowerOffTimeSpan);
}