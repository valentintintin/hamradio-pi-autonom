using System.Reflection;
using System.Text.Json.Serialization;

namespace Monitor.Models.SerialMessages;

[JsonDerivedType(typeof(GpioData))]
[JsonDerivedType(typeof(LoraData))]
[JsonDerivedType(typeof(McuSystemData))]
[JsonDerivedType(typeof(MpptData))]
[JsonDerivedType(typeof(WeatherData))]
public class Message
{
    [JsonIgnore]
    public DateTime ReceivedAt { get; set;  } = DateTime.UtcNow;
    
    [JsonPropertyName("type")]
    public required string Type { get; set; }
    
    public override string ToString()
    {
        var type = GetType();
        PropertyInfo[] properties = type.GetProperties();

        var result = "";
        foreach (var property in properties)
        {
            var propertyName = property.Name;
            var propertyValue = property.GetValue(this);

            result += $"{propertyName}: {propertyValue}\n";
        }

        return result;
    }
}