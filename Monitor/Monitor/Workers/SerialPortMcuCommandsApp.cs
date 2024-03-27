using Monitor.Models.SerialMessages;
using Monitor.Services;

namespace Monitor.Workers;

public class SerialPortMcuCommandsApp : ASerialPortApp
{
    private readonly SerialMessageService _serialMessageService;

    public SerialPortMcuCommandsApp(ILogger<SerialPortMcuCommandsApp> logger, IServiceProvider serviceProvider,
        IConfiguration configuration) : 
        base(logger, serviceProvider, configuration, "SerialPortMessage",
            @"{""type"":""system"",""state"":""started"",""boxOpened"":false}
     {""type"":""mppt"",""batteryVoltage"":12268,""batteryCurrent"":2,""solarVoltage"":163,""solarCurrent"":2,""currentCharge"":0,""status"":136,""night"":true,""alert"":false,""watchdogEnabled"":false,""watchdogPowerOffTime"":10,""watchdogCounter"":0,""powerEnabled"":true,""powerOnVoltage"":11500,""powerOffVoltage"":11300,""statusString"":""NIGHT""}
     {""type"":""weather"",""temperature"":20.50,""humidity"":55,""pressure"":978}
     {""type"":""gpio"",""wifi"":false,""npr"":false,""ldr"":500}
     {""type"":""lora"",""state"":""tx"",""payload"":""F4HVV-15>F4HVV-10,RFONLY:!/7V&-OstcI!!G Solaire camera + NPR70""}
     {""type"":""system"",""state"":""alert"",""boxOpened"":true,""time"":2313942038,""uptime"":20}
     {""type"":""gpio"",""wifi"":false,""npr"":true,""ldr"":300}
     {""type"":""weather"",""temperature"":20.0,""humidity"":50,""pressure"":998}
     {""type"":""mppt"",""batteryVoltage"":12100,""batteryCurrent"":20,""solarVoltage"":1630,""solarCurrent"":20,""currentCharge"":220,""status"":136,""night"":true,""alert"":false,""watchdogEnabled"":false,""watchdogPowerOffTime"":10,""watchdogCounter"":0,""powerEnabled"":true,""powerOnVoltage"":11500,""powerOffVoltage"":11300,""statusString"":""NIGHT""}
     {""type"":""lora"",""state"":""tx"",""payload"":""F4HVV-15>F4HVV,WIDE1-1:!/7V,.Ot#4I!!G Solar NPR+Lora digi+Cam |!!!i!""!>!W!!|""}
     {""type"":""lora"",""state"":""rx"",""payload"":""F4HVV-9>F4HVV: Test !""}")
    {
        _serialMessageService = Services.GetRequiredService<SerialMessageService>();
    }

    protected override async Task MessageReceived(string input)
    {
        if (!input.StartsWith('{') && !input.EndsWith('}'))
        {
            return;
        }
        
        var message = _serialMessageService.ParseMessage(input);
        await MonitorService.UpdateStateFromMessage(message);
    }
}