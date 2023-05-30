using Monitor.Models.SerialMessages;
using Monitor.WorkServices;

namespace Monitor.Workers;

public class SerialPortMessageWorker : ASerialPortWorker
{
    private readonly SerialMessageService _serialMessageService;

    public SerialPortMessageWorker(ILogger<SerialPortMessageWorker> logger, IConfiguration configuration, 
        IServiceScopeFactory serviceScopeFactory) :
        base("SerialPortMessage", logger, configuration, serviceScopeFactory, @"{""type"":""system"",""state"":""started"",""boxOpened"":false}
     {""type"":""time"",""state"":2313942038,""uptime"":2}
     {""type"":""mppt"",""batteryVoltage"":12268,""batteryCurrent"":2,""solarVoltage"":163,""solarCurrent"":2,""currentCharge"":0,""status"":136,""night"":true,""alert"":false,""watchdogEnabled"":false,""watchdogPowerOffTime"":10,""watchdogCounter"":0,""powerEnabled"":true,""powerOnVoltage"":11500,""powerOffVoltage"":11300,""statusString"":""NIGHT""}
     {""type"":""weather"",""temperature"":20.50,""humidity"":55}
     {""type"":""lora"",""state"":""tx"",""payload"":""F4HVV-15>F4HVV-10,RFONLY:!/7V&-OstcI!!G Solaire camera + NPR70""}
     {""type"":""gpio"",""wifi"":false,""npr"":true,""ldr"":300}")
    {
        _serialMessageService = ServiceProvider.GetRequiredService<SerialMessageService>();
    }

    protected override void MessageReceived(string input)
    {
        SerialMessageService.SerialPort ??= SerialPort;
        
        if (input.Contains("Copyright"))
        {
            return;
        }

        Message message = _serialMessageService.ParseMessage(input);
        MonitorService.UpdateStateFromMessage(message);
    }
}