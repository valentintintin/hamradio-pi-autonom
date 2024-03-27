using System.Reactive.Linq;
using System.Reactive.Subjects;
using System.Text.Json;

namespace Monitor.Models;

public interface IStringConfigEntity
{
    string Id { get; }
    bool Retain { get; }
    bool Mqtt { get; }
    bool HasReceivedFromElsewere { get; }

    IObservable<string> ValueStringAsync();
    string ValueAsString();
    bool SetFromStringPayload(string? payload);
    void SetValueToInitialValue();
}

public class ConfigEntity<T> : IStringConfigEntity
{
    public string Id { get; }
    public bool Retain { get; }
    public bool Mqtt { get; }
    public bool HasReceivedFromElsewere { get; private set; }

    public T? Value
    {
        get => ValuePrivate;
        set => SetValue(value);
    }
    private T? ValuePrivate { get; set; }
    private T? OldValue { get; set; }
    
    private readonly Subject<(T? old, T? value)> _valueSubject = new();
    private readonly T? _initialValue;

    public ConfigEntity(string id, bool retain = false, T? initialValue = default, bool mqtt = false)
    {
        Retain = retain;
        Mqtt = mqtt;
        Id = id;
        OldValue = ValuePrivate = _initialValue = initialValue;
        SetValue(initialValue);
    }

    public IObservable<(T? old, T? value, string id)> ValueChanges(bool onlyIfDifferent = true)
    {
        return _valueSubject.AsObservable()
            .Where(v => !onlyIfDifferent || v.old?.Equals(v.value) != true)
            .Select(v => (v.old, v.value, Id));
    }

    public IObservable<string> ValueStringAsync()
    {
        return _valueSubject.AsObservable().Select(v => JsonSerializer.Serialize(v.value));
    }

    public string ValueAsString()
    {
        return JsonSerializer.Serialize(ValuePrivate);
    }

    public void SetValue(T? state)
    {
        OldValue = ValuePrivate;
        ValuePrivate = state;
        
        _valueSubject.OnNext((OldValue, ValuePrivate));
    }

    public bool SetFromStringPayload(string? payload)
    {
        HasReceivedFromElsewere = true;
        
        var newValue = string.IsNullOrWhiteSpace(payload) ? default : JsonSerializer.Deserialize<T?>(payload);

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