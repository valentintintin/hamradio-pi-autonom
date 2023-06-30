using Microsoft.AspNetCore.Mvc;
using Monitor.Models;
using Monitor.Services;

namespace Monitor.Controllers;

[ApiController]
[Route("state")]
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
        MonitorService.State.System = _systemService.GetInfo();
        return MonitorService.State;
    }
}