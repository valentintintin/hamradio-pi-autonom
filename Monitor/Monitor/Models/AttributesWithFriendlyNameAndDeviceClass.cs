using System.Text.Json.Serialization;

namespace Monitor.Models;

public record AttributesWithFriendlyNameAndDeviceClass
{
    [JsonPropertyName("friendly_name")]
    public string? FriendlyName { get; init; }
    
    [JsonPropertyName("device_class")]
    public string? DeviceClass { get; init; }
}