using System.Text.Json;
using Web.Exceptions;
using Web.Models.SerialMessages;

namespace Web.Services;

public class SerialMessageService : AService
{
    public SerialMessageService(ILogger<SerialMessageService> logger) : base(logger)
    {
    }
    
    public Message ParseMessage(string input)
    {
        Message? message, messageTyped;
     
        Logger.LogInformation("Received serial message : {input}", input);
        
        try
        {
            message = JsonSerializer.Deserialize<Message>(input);
        }
        catch (Exception e)
        {
            Logger.LogError(e, "Received serial message : {input} KO. Deserialize impossible", input);

            throw new MessageParseException(e, input);
        }

        if (message == null)
        {
            Logger.LogError("Received serial message : {input} KO. Message null", input);

            throw new MessageParseException(input);
        }

        try
        {
            messageTyped = message.Type switch
            {
                "system" => JsonSerializer.Deserialize<SystemData>(input),
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
            Logger.LogError(e, "Received serial message : {input} KO. Sub Deserialize impossible", input);

            throw new MessageParseException(e, input);
        }

        Logger.LogInformation("Received serial message : {input} OK. {message}", input, messageTyped ?? message);

        return messageTyped ?? message;
    }
}