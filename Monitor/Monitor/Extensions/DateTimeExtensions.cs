namespace Monitor.Extensions;

public static class DateTimeExtensions
{
    public static DateTime ToFrench(this DateTime dateTime)
    {
        TimeZoneInfo timeZone = TimeZoneInfo.FindSystemTimeZoneById("Europe/Paris");
        return TimeZoneInfo.ConvertTimeFromUtc(dateTime, timeZone);
    } 
}