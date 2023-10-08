using System.Reactive.Linq;
using System.Reactive.Subjects;
using System.Text.Json;

namespace Monitor.Models;

public interface IMqttEntity
{
    string Id { get; }
    bool Retain { get; }
    bool HasReceivedFromMqtt { get; }

    IObservable<string> ValueMqttAsync();
    bool SetFromMqttPayload(string? payload);
    void SetValueToInitialValue();
}

public class MqttEntity<T> : IMqttEntity
{
    public string Id { get; }
    public bool Retain { get; }
    public bool HasReceivedFromMqtt { get; private set; }

    public T? Value
    {
        get => ValuePrivate;
        set => SetValue(value);
    }
    private T? ValuePrivate { get; set; }
    private T? OldValue { get; set; }
    
    private readonly Subject<(T? old, T? value)> _valueSubject = new();
    private readonly T? _initialValue;

    public MqttEntity(string id, bool retain = false, T? initialValue = default)
    {
        Retain = retain;
        Id = id;
        OldValue = ValuePrivate = _initialValue = initialValue;
        SetValue(initialValue);
    }

    public IObservable<(T? old, T? value)> ValueChanges(bool onlyIfDifferent = true)
    {
        return _valueSubject.AsObservable().Where(v => !onlyIfDifferent || v.old?.Equals(v.value) != true);
    }

    public IObservable<string> ValueMqttAsync()
    {
        return _valueSubject.AsObservable().Skip(1).Select(v => JsonSerializer.Serialize(v.value));
    }

    public void SetValue(T? state)
    {
        OldValue = ValuePrivate;
        ValuePrivate = state;
        
        _valueSubject.OnNext((OldValue, ValuePrivate));
    }

    public bool SetFromMqttPayload(string? payload)
    {
        HasReceivedFromMqtt = true;
        
        T? newValue = string.IsNullOrWhiteSpace(payload) ? default : JsonSerializer.Deserialize<T?>(payload);

        if (newValue?.Equals(ValuePrivate) != true)
        {
            SetValue(newValue);

            return true;
        }

        return false;
    }

    public void SetValueToInitialValue()
    {
        Value = _initialValue;
    }

    public bool IsTrue()
    {
        return ValuePrivate?.ToString() == bool.TrueString;
    }

    public bool IsFalse()
    {
        return ValuePrivate?.ToString() == bool.FalseString;
    }

    public IObservable<bool> IsTrueAsync()
    {
        return ValueChanges().Select(v => v.value?.ToString() == bool.TrueString).Where(v => v);
    }

    public IObservable<bool> IsFalseAsync()
    {
        return ValueChanges().Select(v => v.value?.ToString() == bool.FalseString).Where(v => v);
    }

    public override string ToString()
    {
        return $"{Id} => {Value}";
    }
}