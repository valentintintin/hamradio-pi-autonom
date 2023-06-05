namespace Monitor.Workers;

public class SerialPortLogWorker : ASerialPortWorker
{
    public SerialPortLogWorker(ILogger<SerialPortLogWorker> logger, IConfiguration configuration, 
        IServiceScopeFactory serviceScopeFactory) : 
        base("SerialPortLog", logger, configuration, serviceScopeFactory, 
            @"E: [MPPT] Charger error
I: [WEATHER] Temperature 22.80C Humidity=60
I: [LORA_TX] Start send : F4HVV-15>F4HVV-10,RFONLY:!/7V&-OstcI!!G Solaire camera + NPR70
I: [LORA_TX] Start send : F4HVV-15>F4HVV-10,RFONLY::F4HVV-15 :PARM.Battery,ICharg,Temp,Humdt,Slep,Night,Alrt,WDog,Wifi,5V,Box, ,
I: [LORA_TX] Start send : F4HVV-15>F4HVV-10,RFONLY::F4HVV-15 :UNIT.V,mA,C,%,min, , , , , , , ,
I: [LORA_TX] Start send : F4HVV-15>F4HVV-10,RFONLY:T#0,0,0,22.80,60,0,00000000 Chg:NIGHT Up:11s
I: [TIME] 2003-10-17T23:47:09 Uptime 47s
E: [MPPT] Charger error
I: [WEATHER] Temperature 22.80C Humidity=61
I: [TIME] 2003-10-17T23:47:19 Uptime 57s
E: [MPPT] Charger error
I: [GPIO] LDR (17) is 4095
W: [SYSTEM] Box opened !
I: [LORA_TX] Start send : F4HVV-15>F4HVV-10,RFONLY::F4HVV-10 :Box opened !{1
I: [WEATHER] Temperature 22.80C Humidity=61")
    {
    }

    protected override Task MessageReceived(string input)
    {
        MonitorService.AddLog(input);

        return Task.CompletedTask;
    }
}