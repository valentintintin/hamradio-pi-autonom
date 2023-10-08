using System.Reflection;
using System.Text.Json.Serialization;

namespace Monitor.Models.SerialMessages;

[JsonDerivedType(typeof(GpioData))]
[JsonDerivedType(typeof(LoraData))]
[JsonDerivedType(typeof(McuSystemData))]
[JsonDerivedType(typeof(MpptData))]
[JsonDerivedType(typeof(TimeData))]
[JsonDerivedType(typeof(WeatherData))]
public class Message
{
    [JsonIgnore]
    public DateTime ReceivedAt { get; } = DateTime.UtcNow;
    
    [JsonPropertyName("type")]
    public required string Type { get; set; }
    
    public override string ToString()
    {
        Type type = GetType();
        PropertyInfo[] properties = type.GetProperties();

        string result = "";
        foreach (PropertyInfo property in properties)
        {
            string propertyName = property.Name;
            object? propertyValue = property.GetValue(this);

            result += $"{propertyName}: {propertyValue}\n";
        }

        return result;
    }
}