using System.Reactive.Concurrency;
using Monitor.Models.SerialMessages;
using Monitor.WorkServices;
using NetDaemon.AppModel;
using NetDaemon.HassModel;

namespace Monitor.Apps;

[NetDaemonApp(Id = "serial_port_message_app")]
public class SerialPortMessageApp : ASerialPortApp
{
    private readonly SerialMessageService _serialMessageService;

    public SerialPortMessageApp(IHaContext ha, ILogger<SerialPortMessageApp> logger, EntitiesManagerService entitiesManagerService,
        MonitorService monitorService, IConfiguration configuration, IScheduler scheduler, SerialMessageService serialMessageService) : 
        base(ha, logger, entitiesManagerService, monitorService, configuration, scheduler, "SerialPortMessage",
            @"{""type"":""system"",""state"":""started"",""boxOpened"":false}
     {""type"":""time"",""state"":2313942038,""uptime"":2}
     {""type"":""mppt"",""batteryVoltage"":12268,""batteryCurrent"":2,""solarVoltage"":163,""solarCurrent"":2,""currentCharge"":0,""status"":136,""night"":true,""alert"":false,""watchdogEnabled"":false,""watchdogPowerOffTime"":10,""watchdogCounter"":0,""powerEnabled"":true,""powerOnVoltage"":11500,""powerOffVoltage"":11300,""statusString"":""NIGHT""}
     {""type"":""weather"",""temperature"":20.50,""humidity"":55}
     {""type"":""gpio"",""wifi"":false,""npr"":false,""ldr"":500}
     {""type"":""lora"",""state"":""tx"",""payload"":""F4HVV-15>F4HVV-10,RFONLY:!/7V&-OstcI!!G Solaire camera + NPR70""}
     {""type"":""system"",""state"":""alert"",""boxOpened"":true}
     {""type"":""gpio"",""wifi"":false,""npr"":true,""ldr"":300}
     {""type"":""time"",""state"":2313942055,""uptime"":20}
     {""type"":""weather"",""temperature"":20.0,""humidity"":50}
     {""type"":""mppt"",""batteryVoltage"":12100,""batteryCurrent"":20,""solarVoltage"":1630,""solarCurrent"":20,""currentCharge"":220,""status"":136,""night"":true,""alert"":false,""watchdogEnabled"":false,""watchdogPowerOffTime"":10,""watchdogCounter"":0,""powerEnabled"":true,""powerOnVoltage"":11500,""powerOffVoltage"":11300,""statusString"":""NIGHT""}")
    {
        _serialMessageService = serialMessageService;
    }

    protected override async Task MessageReceived(string input)
    {
        SerialMessageService.SerialPort ??= SerialPort;
        
        if (input.Contains("Copyright"))
        {
            return;
        }

        Message message = _serialMessageService.ParseMessage(input);
        await MonitorService.UpdateStateFromMessage(message);
    }
}