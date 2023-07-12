using System.Reactive.Linq;
using System.Reactive.Subjects;
using System.Text.Json;

namespace Monitor.Models;

public interface IMqttEntity
{
    string Id { get; }
    bool Retain { get; }
    bool HasReceivedInitialValueFromMqtt { get; }

    IObservable<(string? old, string value)> ValueMqttAsync();

    bool SetFromMqttPayload(string? payload);

    void EmitToMqtt();
}

public class MqttEntity<T> : IMqttEntity
{
    public string Id { get; }
    public bool Retain { get; }

    public bool HasReceivedInitialValueFromMqtt { get; private  set; }
    
    public T? Value
    {
        get => _value;
        set => SetValue(value);
    }
    private T? _value { get; set; } 
    private T? _oldValue { get; set; } 
        
    private readonly Subject<(T? old, T? value)> _valueLocal = new();
    private readonly Subject<(string? old, string? value)> _valueMqtt = new();

    public MqttEntity(string id, bool retain = false, T? initialValue = default)
    {
        Retain = retain;
        Id = id;

        if (retain && initialValue != null)
        {
            _value = initialValue;
            _valueLocal.OnNext((default, _value));
        }
    }

    public IObservable<(T? old, T? value)> ValueChanges(bool onlyIfDifferent = true)
    {
        return _valueLocal.AsObservable().Where(v => !onlyIfDifferent || v.old?.Equals(v.value) != true);
    }

    public IObservable<(string? old, string value)> ValueMqttAsync()
    {
        return _valueMqtt.AsObservable().Select(s => (s.old, s.value ?? string.Empty));
    }

    public void SetValue(T? state)
    {
        _oldValue = _value;
        _value = state;
        _valueLocal.OnNext((_oldValue, _value));
        EmitToMqtt();
    }

    public bool SetFromMqttPayload(string? payload)
    {
        HasReceivedInitialValueFromMqtt = true;

        T? newValue = string.IsNullOrWhiteSpace(payload) ? default : JsonSerializer.Deserialize<T?>(payload);

        if (newValue?.Equals(_value) != true)
        {
            SetValue(newValue);

            return true;
        }

        return false;
    }

    public void EmitToMqtt()
    {
        _valueMqtt.OnNext((JsonSerializer.Serialize(_oldValue), JsonSerializer.Serialize(_value)));
    }

    public bool IsTrue()
    {
        return _value?.ToString() == bool.TrueString;
    }

    public bool IsFalse()
    {
        return _value?.ToString() == bool.FalseString;
    }

    public IObservable<bool> IsTrueAsync()
    {
        return ValueChanges().Select(v => v.value?.ToString() == bool.TrueString).Where(v => v);
    }

    public IObservable<bool> IsFalseAsync()
    {
        return ValueChanges().Select(v => v.value?.ToString() == bool.FalseString).Where(v => v);
    }
}