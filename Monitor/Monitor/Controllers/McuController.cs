using Microsoft.AspNetCore.Mvc;
using Monitor.Models;
using Monitor.Services;

namespace Monitor.Controllers;

[ApiController]
[Route("mcu")]
public class McuController : AController
{
    private readonly SerialMessageService _serialMessageService;

    public McuController(ILogger<McuController> logger, SerialMessageService serialMessageService) : base(logger)
    {
        _serialMessageService = serialMessageService;
    }

    [Route("serial_tx")]
    public IActionResult SerialTx(string payload)
    {
        Logger.LogInformation("Request to send by Serial to MCU : {payload}", payload);

        _serialMessageService.SendCommand(payload);
        
        return NoContent();
    }
}