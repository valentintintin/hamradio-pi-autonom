namespace Monitor.Extensions;

public static class DateTimeExtensions
{
    public static DateTime ToFrench(this DateTime dateTime)
    {
        var timeZone = TimeZoneInfo.FindSystemTimeZoneById("Europe/Paris");
        return TimeZoneInfo.ConvertTimeFromUtc(dateTime, timeZone);
    } 
}