using System.ComponentModel.DataAnnotations;

namespace Monitor.Context.Entities;

public class Config : IEntity
{
    public long Id { get; set; }
    public DateTime CreatedAt { get; set; }
    
    [MaxLength(64)]
    public required string Name { get; set; }
    
    [MaxLength(128)]
    public required string Value { get; set; }
}