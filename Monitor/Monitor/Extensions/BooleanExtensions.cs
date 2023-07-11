namespace Monitor.Extensions;

public static class BooleanExtensions
{
    public static string ToFrench(this bool value)
    {
        return value ? "Activé" : "Désactivé";
    }
}