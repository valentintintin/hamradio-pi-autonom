namespace Monitor.Extensions;

public static class TimeSpanExtensions
{
    public static DateTime ToDateTime(this TimeSpan timeSpan)
    {
        return DateTime.UtcNow.Date.Add(timeSpan);
    }
    
    public static DateTime? ToDateTime(this TimeSpan? timeSpan)
    {
        return timeSpan?.ToDateTime();
    }
}