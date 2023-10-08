using Microsoft.AspNetCore.Mvc;
using Monitor.Models;
using Monitor.Services;

namespace Monitor.Controllers;

[ApiController]
[Route("system_info")]
public class SystemInfoController : AController
{
    private readonly MonitorService _monitorService;

    public SystemInfoController(ILogger<SystemInfoController> logger, MonitorService monitorService) : base(logger)
    {
        _monitorService = monitorService;
    }

    [HttpPost]
    public NoContentResult Post(SystemState? systemState)
    {
        _monitorService.UpdateSystemState(systemState);

        return NoContent();
    }
}