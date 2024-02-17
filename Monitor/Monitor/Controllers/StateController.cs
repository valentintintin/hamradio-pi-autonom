using Microsoft.AspNetCore.Mvc;
using Monitor.Models;
using Monitor.Services;

namespace Monitor.Controllers;

[ApiController]
[Route("states")]
public class StateController(
    ILogger<StateController> logger,
    SystemService systemService,
    MonitorService monitorService)
    : AController(logger)
{
    [HttpGet]
    public MonitorState Index()
    {
        return MonitorService.State;
    }

    [HttpPost("system_info")]
    public NoContentResult Post(SystemState? systemState)
    {
        monitorService.UpdateSystemState(systemState);

        return NoContent();
    }

    [HttpGet("datetime")]
    public DateTime DateTime(DateTime datetime)
    {
        systemService.SetTime(datetime);

        return System.DateTime.UtcNow;
    }
}