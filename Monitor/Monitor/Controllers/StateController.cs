using Microsoft.AspNetCore.Mvc;
using Monitor.Models;
using Monitor.Services;

namespace Monitor.Controllers;

[ApiController]
[Route("state")]
public class StateController : AController
{
    public StateController(ILogger<StateController> logger) : base(logger)
    {
    }

    [HttpGet]
    public MonitorState Index()
    {
        return MonitorService.State;
    }
}