namespace Monitor.Context.Entities;

public class System : IEntity
{
    public long Id { get; set; }
    public DateTime CreatedAt { get; set; }
    
    public bool Npr { get; set; }
    
    public bool Wifi { get; set; }
    
    public bool BoxOpened { get; set; }
    
    public long Uptime { get; set; }
    
    public long McuUptime { get; set; }
    
    public float TemperatureRtc { get; set; }
}