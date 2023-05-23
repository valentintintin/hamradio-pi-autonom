using System.ComponentModel.DataAnnotations;

namespace Web.Context.Entities;

public class Weather : IEntity
{
    [Key]
    public long Id { get; set; }
    public DateTime CreatedAt { get; set; }
    
    public double Temperature { get; set; }
    
    public int Humidity { get; set; } 
}