using Microsoft.AspNetCore.Mvc;

namespace Monitor.Controllers;

public abstract class AController : Controller
{
    protected readonly ILogger<AController> Logger;

    protected AController(ILogger<AController> logger)
    {
        Logger = logger;
    }
}