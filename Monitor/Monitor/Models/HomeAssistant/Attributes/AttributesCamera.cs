using System.Text.Json.Serialization;

namespace Monitor.Models.HomeAssistant.Attributes;

public record AttributesCamera
{
    [JsonPropertyName("entity_picture")]
    public string? EntityPicture { get; init; }
    
    [JsonPropertyName("token")]
    public string? Token { get; init; }
}