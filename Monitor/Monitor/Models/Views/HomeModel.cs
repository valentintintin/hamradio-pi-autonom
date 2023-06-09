namespace Monitor.Models.Views;

public class HomeModel
{
    public required MonitorState State { get; set; }
    
    public string? LastPhoto { get; set; }
    
    public TimeSpan Uptime { get; set; }
}