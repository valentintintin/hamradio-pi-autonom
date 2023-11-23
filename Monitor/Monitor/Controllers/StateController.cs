using Microsoft.AspNetCore.Mvc;
using Monitor.Models;
using Monitor.Services;

namespace Monitor.Controllers;

[ApiController]
[Route("states")]
public class StateController : AController
{
    private readonly SystemService _systemService;

    public StateController(ILogger<StateController> logger, SystemService systemService) : base(logger)
    {
        _systemService = systemService;
    }

    [HttpGet]
    public MonitorState Index()
    {
        return MonitorService.State;
    }

    [HttpGet("shutdown")]
    public IActionResult NeedShutdown()
    {
        Logger.LogDebug("Request if shutdown asked from API. ShutdownAsked : {shutdownAsked}", _systemService.IsShutdownAsked());

        if (_systemService.IsShutdownAsked()) // TODO fixme !!
        {
            return NoContent();
        }

        return BadRequest();
    }

    [HttpGet("datetime")]
    public IActionResult DateTime()
    {
        DateTime? dateTime = _systemService.ChangeDateTime;

        _systemService.ChangeDateTime = null;

        Logger.LogDebug("Request if change datetime needed from API. DateTime to set : {DateTime}", dateTime);

        if (dateTime.HasValue)
        {        
            return Ok(dateTime.Value.ToString("s"));   
        }

        return BadRequest();
    }
}