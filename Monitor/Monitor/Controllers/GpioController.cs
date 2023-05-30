using Microsoft.AspNetCore.Mvc;
using Monitor.WorkServices;

namespace Monitor.Controllers;

[Route("gpio")]
public class GpioController : AController
{
    private readonly SerialMessageService _serialMessageService;

    public GpioController(ILogger<GpioController> logger, SerialMessageService serialMessageService) : base(logger)
    {
        _serialMessageService = serialMessageService;
    }

    [Route("wifi/{enabled}")]
    public void Wifi(bool enabled)
    {
        Logger.LogInformation("From web, set wifi to {enabled}", enabled);
        
        _serialMessageService.SetWifi(enabled);
    }

    [Route("npr/{enabled}")]
    public void Npr(bool enabled)
    {
        Logger.LogInformation("From web, set npr to {enabled}", enabled);
        
        _serialMessageService.SetNpr(enabled);
    }
}