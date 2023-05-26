using Monitor.Models.SerialMessages;

namespace Monitor.Models;

public class MonitorState
{
    public readonly GpioState Gpio = new();
    
    public WeatherData Weather { get; set; } = new ();

    public TimeData Time { get; set; } = new();

    public McuSystemData McuSystem = new();

    public MpptData Mppt { get; set; } = new();

    public LoraState Lora { get; } = new();
    
    public SystemState System { get; set; } = new();

    public readonly LimitedList<Message> LastMessagesReceived = new(20);
    
    public readonly LimitedList<string> LastLogReceived = new(50);
}