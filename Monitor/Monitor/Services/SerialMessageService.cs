using System.IO.Ports;
using System.Text.Json;
using Monitor.Exceptions;
using Monitor.Models.SerialMessages;

namespace Monitor.Services;

public class SerialMessageService : AService
{
    public static SerialPort? SerialPort;
    
    public SerialMessageService(ILogger<SerialMessageService> logger) : base(logger)
    {
    }
    
    public Message ParseMessage(string input)
    {
        Message? message, messageTyped;
        
        try
        {
            message = JsonSerializer.Deserialize<Message>(input);
        }
        catch (Exception e)
        {
            if (input.Contains("lora")) // because APRS can have char " in string and we do not escape it in C++
            {
                string payloadString = "payload\":\"";
                
                message = new LoraData
                {
                    Type = "lora",
                    State = input.Contains("tx") ? "tx" : "rx",
                    Payload = input[(input.IndexOf(payloadString, StringComparison.Ordinal) + payloadString.Length)..input.LastIndexOf('"')]
                };

                input = JsonSerializer.Serialize(message, typeof(LoraData));
            }
            else
            {
                Logger.LogError(e, "Received serial message KO. Deserialize impossible : {input}", input);

                throw new MessageParseException(e, input);
            }
        }

        if (message == null)
        {
            Logger.LogError("Received serial message KO. Message null : {input}", input);

            throw new MessageParseException(input);
        }

        try
        {
            messageTyped = message.Type switch
            {
                "system" => JsonSerializer.Deserialize<McuSystemData>(input),
                "weather" => JsonSerializer.Deserialize<WeatherData>(input),
                "time" => JsonSerializer.Deserialize<TimeData>(input),
                "mppt" => JsonSerializer.Deserialize<MpptData>(input),
                "lora" => JsonSerializer.Deserialize<LoraData>(input),
                "gpio" => JsonSerializer.Deserialize<GpioData>(input),
                _ => null
            };
        }
        catch (Exception e)
        {
            Logger.LogError(e, "Received serial message KO. Sub Deserialize impossible : {input}", input);

            throw new MessageParseException(e, input);
        }

        Logger.LogInformation("Received serial message OK : {input}", input);

        return messageTyped ?? message;
    }

    public void SetPowerOnOffVoltage(int powerOnVoltage, int powerOffVoltage)
    {
        SendCommand($"pow {powerOnVoltage} {powerOffVoltage}");
    }

    public void SetWatchdog(TimeSpan timeOff)
    {
        SendCommand($"dog {timeOff.TotalSeconds}");
    }

    public void SetWifi(bool enabled)
    {
        SendCommand($"wifi {(enabled ? "1" : "0")}");
    }

    public void SetNpr(bool enabled)
    {
        SendCommand($"npr {(enabled ? "1" : "0")}");
    }

    public void SendTelemetry()
    {
        SendCommand("telem");
    }

    public void SendTelemetryParams()
    {
        SendCommand("telemParam");
    }

    public void ResetMcu()
    {
        SendCommand("reset");
    }

    public void SetWatchdogSafety(bool enabled)
    {
        SetEepromMcu(0x01, enabled ? 1 : 0);
    }

    public void SendLora(string message)
    {
        if (string.IsNullOrWhiteSpace(message))
        {
            return;
        }
        
        SendCommand($"lora \"{message}\"");
    }

    private void SetEepromMcu(int address, int value)
    {
        SendCommand($"set {address} {value}");
    }

    public void SendCommand(string command)
    {
        if (SerialPort == null)
        {
            Logger.LogError("Send Serial Command impossible {command}", command);
            return;
        }

        Logger.LogInformation("Send Serial Command {command}", command);

        try
        {
            SerialPort.WriteLine(command);
        }
        catch (Exception e)
        {
            Logger.LogError(e, "Send Serial Command in error {command}", command);
        }
    }
}