using Monitor.Models.SerialMessages;

namespace Monitor.Models;

public class MonitorState
{
    public GpioData Gpio { get; set; } = new()
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

    public McuSystemData McuSystem { get; set; } = new()
    {
        State = "no data",
        Type = "system"
    };

    public MpptData Mppt { get; set; } = new()
    {
        Type = "mppt"
    };

    public LoraState Lora { get; } = new();

    public SystemState? System { get; set; }
    
    public LimitedList<Message> LastMessagesReceived { get; } = new(15);
    
    public LimitedList<string> LastLogReceived { get; }= new(30);
}