using Microsoft.AspNetCore.Mvc;
using Monitor.Models.Views;
using Monitor.WorkServices;

namespace Monitor.Controllers;

public class HomeController : AController
{
    private readonly CameraService _cameraService;

    public HomeController(ILogger<HomeController> logger, CameraService cameraService) : base(logger)
    {
        _cameraService = cameraService;
    }

    public IActionResult Index()
    {
        return View(new HomeModel
        {
            State = MonitorService.State,
            LastPhoto = _cameraService.GetLast()
        });
    }
}