using System.Text.Json.Serialization;

namespace Monitor.Models;

public class SystemState
{
    [JsonPropertyName("uptime")]
    public long Uptime { get; set; }
    
    public TimeSpan UptimeTimeSpan => TimeSpan.FromSeconds(Uptime);
    
    [JsonPropertyName("cpu")]
    public int Cpu { get; set; }
    
    [JsonPropertyName("ram")]
    public int Ram { get; set; }
    
    [JsonPropertyName("disk")]
    public int Disk { get; set; }
}