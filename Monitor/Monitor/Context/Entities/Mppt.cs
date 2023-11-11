using System.ComponentModel.DataAnnotations;

namespace Monitor.Context.Entities;

public class Mppt : IEntity
{
    [Key]
    public long Id { get; set; }
    public DateTime CreatedAt { get; set; }
    
    public int BatteryVoltage { get; set; }

    public int BatteryCurrent { get; set; }

    public int SolarVoltage { get; set; }

    public int SolarCurrent { get; set; }

    public int CurrentCharge { get; set; }

    public int Status { get; set; }
    
    public string? StatusString { get; set; }

    public bool Night { get; set; }

    public bool Alert { get; set; }

    public bool WatchdogEnabled { get; set; }

    public long WatchdogPowerOffTime { get; set; } 
    
    public float Temperature { get; set; }
}