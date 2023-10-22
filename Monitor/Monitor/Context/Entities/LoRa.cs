using System.ComponentModel.DataAnnotations;

namespace Monitor.Context.Entities;

public class LoRa : IEntity
{
    public long Id { get; set; }
    public DateTime CreatedAt { get; set; }
    
    [MaxLength(16)]
    public string? Sender { get; set; }
    
    [MaxLength(300)]
    public required string Frame { get; set; }
    
    public required bool IsTx { get; set; }
}