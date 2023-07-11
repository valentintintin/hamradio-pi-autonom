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
            Logger.LogError(e, "Received serial message KO. Deserialize impossible : {input}", input);

            throw new MessageParseException(e, input);
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
        SendCommand($"wifi {(enabled ? "on" : "off")}");
    }

    public void SetNpr(bool enabled)
    {
        SendCommand($"npr {(enabled ? "on" : "off")}");
    }

    public void SendTelemetry()
    {
        SendCommand("telem");
    }

    public void SendLora(string message)
    {
        if (string.IsNullOrWhiteSpace(message))
        {
            return;
        }
        
        SendCommand($"lora \"{message}\"");
    }

    public void SetTime(DateTimeOffset dateTime)
    {
        SendCommand($"time {dateTime.ToUnixTimeSeconds()}");
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