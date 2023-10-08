// ReSharper disable InconsistentNaming
namespace Monitor.Models;

public class LineChartData<T>
{
    public required string date { get; init; }
    
    public required string type { get; init; }
    
    public required T value { get; init; }
}