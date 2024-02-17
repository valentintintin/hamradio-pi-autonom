using Microsoft.AspNetCore.Mvc;

namespace Monitor.Controllers;

public abstract class AController(ILogger<AController> logger) : Controller
{
    protected readonly ILogger<AController> Logger = logger;
}