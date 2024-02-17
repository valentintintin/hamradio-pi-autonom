namespace Monitor.Extensions;

public static class StringExtensions
{
    public static int ToInt(this string? value, int? defaultValue = null)
    {
        if (!int.TryParse(value, out var valueLong))
        {
            if (defaultValue.HasValue)
            {
                return defaultValue.Value;
            }
            
            throw new ArgumentException($"{value} is not a number");
        }

        return valueLong;
    }
    
    public static int? ToIntNullable(this string? value)
    {
        if (!int.TryParse(value, out var valueLong))
        {
            return null;
        }

        return valueLong;
    }
    
    public static long ToLong(this string? value, long? defaultValue = null)
    {
        if (!long.TryParse(value, out var valueLong))
        {
            if (defaultValue.HasValue)
            {
                return defaultValue.Value;
            }

            throw new ArgumentException($"{value} is not a number");
        }

        return valueLong;
    }
    
    public static long? ToLongNullable(this string? value)
    {
        if (!long.TryParse(value, out var valueLong))
        {
            return null;
        }

        return valueLong;
    }
}