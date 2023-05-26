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

    [JsonPropertyName("night")]
    public bool Night { get; set; }

    [JsonPropertyName("alert")]
    public bool Alert { get; set; }

    [JsonPropertyName("watchdogEnabled")]
    public bool WatchdogEnabled { get; set; }

    [JsonPropertyName("watchdogPowerOffTime")]
    public long WatchdogPowerOffTime { get; set; }

    [JsonPropertyName("watchdogCounter")]
    public int WatchdogCounter { get; set; }

    [JsonPropertyName("powerEnabled")]
    public bool PowerEnabled { get; set; }

    [JsonPropertyName("powerOffVoltage")]
    public int PowerOffVoltage { get; set; }

    [JsonPropertyName("powerOnVoltage")]
    public int PowerOnVoltage { get; set; }

    public TimeSpan WatchdogCounterTimeSpan => TimeSpan.FromSeconds(WatchdogCounter);
    public TimeSpan WatchdogPowerOffTimeSpan => TimeSpan.FromSeconds(WatchdogPowerOffTime);
    public DateTime WatchdogPowerOffDateTime => DateTime.UtcNow.Add(WatchdogPowerOffTimeSpan);

    public override string ToString()
    {
        return $"{base.ToString()} is {(PowerEnabled ? "powered" : "off")}. " +
               $"Battery is {BatteryVoltage}mV, Solar current is {SolarCurrent}mA and balance is {CurrentCharge}mA." +
               $"{(Alert ? " Alert triggered." : "")}" + $"{(Night ? " It's night" : " It's day")}" +
               $" Watchdog {(WatchdogEnabled ? "enabled" : "disabled")} to poweroff for {WatchdogPowerOffTimeSpan}. Current counter : {WatchdogCounterTimeSpan}" +
			   $" Power off voltage : {PowerOffVoltage}mV Power on voltage : {PowerOnVoltage}mV"
               ;
    }
}