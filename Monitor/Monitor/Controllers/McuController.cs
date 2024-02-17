using Microsoft.AspNetCore.Mvc;
using Monitor.Models;
using Monitor.Services;

namespace Monitor.Controllers;

[ApiController]
[Route("mcu")]
public class McuController(ILogger<McuController> logger, SerialMessageService serialMessageService)
    : AController(logger)
{
    [Route("serial_tx")]
    public IActionResult SerialTx(string payload)
    {
        Logger.LogInformation("Request to send by Serial to MCU : {payload}", payload);

        serialMessageService.SendCommand(payload);
        
        return NoContent();
    }
}