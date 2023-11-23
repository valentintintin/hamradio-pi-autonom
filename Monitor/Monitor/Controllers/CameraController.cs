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
    public ActionResult GetLast()
    {
        string? lastFullPath = _cameraService.GetFinalLast();

        if (string.IsNullOrWhiteSpace(lastFullPath))
        {
            return new NotFoundResult();
        }
        
        Response.Headers.Add("Content-Disposition", new ContentDisposition
        {
            FileName = "last.jpg",
            Inline = true
        }.ToString());

        return new PhysicalFileResult(lastFullPath, "image/jpeg");
    }

    [HttpGet("current.jpg")]
    public async Task<FileStreamResult> GetCurrent([FromQuery] bool save = false)
    {
        Response.Headers.Add("Content-Disposition", new ContentDisposition
        {
            FileName = "last.jpg",
            Inline = true
        }.ToString());
        
        return new FileStreamResult(await _cameraService.CreateFinalImageFromLasts(save), "image/jpeg");
    }
}