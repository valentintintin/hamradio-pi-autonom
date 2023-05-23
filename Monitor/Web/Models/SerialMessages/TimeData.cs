using System.Text.Json.Serialization;

namespace Web.Models.SerialMessages;

public class TimeData : Message
{
    [JsonPropertyName("state")]
    public long Timestamp { get; set; }

    [JsonPropertyName("uptime")] 
    public long Uptime { get; set; }
    
    public DateTimeOffset DateTime => DateTimeOffset.FromUnixTimeSeconds(Timestamp);
    public TimeSpan UptimeTimeSpan => TimeSpan.FromSeconds(Uptime);

    public override string ToString()
    {
        return $"{base.ToString()} is {DateTime} and running for {UptimeTimeSpan}";
    }
}