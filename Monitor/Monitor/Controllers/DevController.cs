using System.Globalization;
using Microsoft.AspNetCore.Mvc;
using Monitor.Services;

namespace Monitor.Controllers;

[ApiController]
[Route("dev")]
public class DevController : AController
{
    public DevController(ILogger<DevController> logger) : base(logger)
    {
    }

    [HttpGet("test")]
    public object? Test()
    {
        return default;
    }
}