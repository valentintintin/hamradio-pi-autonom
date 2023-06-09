using Microsoft.AspNetCore.Mvc;
using Monitor.WorkServices;

namespace Monitor.Controllers;

[ApiController]
[Route("dev")]
public class DevController : AController
{
    public DevController(ILogger<DevController> logger) : base(logger)
    {
    }

    [Route("test")]
    public void Test()
    {
        Logger.LogTrace("Test route accessed");
    }
}