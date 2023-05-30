using Monitor.Models.SerialMessages;

namespace Monitor.Models;

public class MonitorState
{
    public GpioData Gpio = new()
    {
        Type = "gpio"
    };
    
    public WeatherData Weather { get; set; } = new ()
    {
        Type = "weather"
    };

    public TimeData Time { get; set; } = new()
    {
        Type = "time"
    };

    public McuSystemData McuSystem = new()
    {
        State = "no data",
        Type = "system"
    };

    public MpptData Mppt { get; set; } = new()
    {
        Type = "mppt"
    };

    public LoraState Lora { get; } = new();
    
    public SystemState System { get; set; } = new();

    public readonly LimitedList<Message> LastMessagesReceived = new(20);
    
    public readonly LimitedList<string> LastLogReceived = new(50);
}