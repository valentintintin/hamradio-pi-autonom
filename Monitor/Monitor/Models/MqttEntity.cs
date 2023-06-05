using System.Globalization;
using NetDaemon.HassModel;
using NetDaemon.HassModel.Entities;

namespace Monitor;

public record MqttEntity : Entity
{
    public string? NewState { get; private set; }

    public MqttEntity(IHaContext haContext, string entityId) : base(haContext, entityId)
    {
    }

    public void SetState(string? value)
    {
        NewState = value;
    }

    public void SetState(bool? value)
    {
        NewState = value == true ? "ON" : "OFF";
    }

    public void SetState(int? value)
    {
        NewState = value?.ToString();
    }

    public void SetState(long? value)
    {
        NewState = value?.ToString();
    }

    public void SetState(float? value)
    {
        NewState = value?.ToString(CultureInfo.InvariantCulture);
    }

    public void SetState(double? value)
    {
        NewState = value?.ToString(CultureInfo.InvariantCulture);
    }

    public void SetState(DateTime? value)
    {
        NewState = value?.ToString("O");
    }

    public void SetState(TimeSpan? value)
    {
        SetState(value?.TotalSeconds);
    }

    public void ClearState()
    {
        NewState = null;
    }
}