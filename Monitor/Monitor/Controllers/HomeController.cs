using Microsoft.AspNetCore.Mvc;
using Monitor.Models.Views;
using Monitor.WorkServices;
using NetDaemon.HassModel;
using NetDaemon.HassModel.Entities;

namespace Monitor.Controllers;

public class HomeController : AController
{
    private readonly CameraService _cameraService;
    private readonly SystemService _systemService;

    public HomeController(ILogger<HomeController> logger, CameraService cameraService, SystemService systemService) : base(logger)
    {
        _cameraService = cameraService;
        _systemService = systemService;
    }

    public IActionResult Index()
    {
        return View(new HomeModel
        {
            State = MonitorService.State,
            LastPhoto = _cameraService.GetFinalLast(),
            Uptime =  _systemService.GetUptime()
        });
    }
}