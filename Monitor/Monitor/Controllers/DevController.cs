using System.Globalization;
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
    public TimeSpan Test()
    {
        return DateTime.UtcNow - DateTime.Parse(EntitiesManagerService.Entities.SunRising!.State!, CultureInfo.CurrentCulture, DateTimeStyles.AdjustToUniversal);
    }
}