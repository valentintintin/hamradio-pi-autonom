using System.Net.Mime;
using Microsoft.AspNetCore.Mvc;
using Monitor.Services;

namespace Monitor.Controllers;

[ApiController]
[Route("camera")]
public class CameraController : AController
{
    private readonly CameraService _cameraService;

    public CameraController(ILogger<CameraController> logger, CameraService cameraService) : base(logger)
    {
        _cameraService = cameraService;
    }

    [HttpGet("last.jpg")]
    public FileStreamResult GetLast()
    {
        Response.Headers.Add("Content-Disposition", new ContentDisposition
        {
            FileName = "last.jpg",
            Inline = true
        }.ToString());
        
        return new FileStreamResult(_cameraService.CreateFinalImageFromLasts(false), "image/jpeg");
    }
}