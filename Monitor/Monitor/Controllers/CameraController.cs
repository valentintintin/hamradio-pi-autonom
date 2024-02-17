using System.Net.Mime;
using Microsoft.AspNetCore.Mvc;
using Monitor.Services;

namespace Monitor.Controllers;

[ApiController]
[Route("camera")]
public class CameraController(ILogger<CameraController> logger, CameraService cameraService)
    : AController(logger)
{
    [HttpGet("last.jpg")]
    public ActionResult GetLast()
    {
        var lastFullPath = cameraService.GetFinalLast();

        if (string.IsNullOrWhiteSpace(lastFullPath))
        {
            return new NotFoundResult();
        }
        
        Response.Headers.Append("Content-Disposition", new ContentDisposition
        {
            FileName = "last.jpg",
            Inline = true
        }.ToString());

        return new PhysicalFileResult(lastFullPath, "image/jpeg");
    }

    [HttpGet("current.jpg")]
    public async Task<FileStreamResult> GetCurrent([FromQuery] bool save = false)
    {
        Response.Headers.Append("Content-Disposition", new ContentDisposition
        {
            FileName = "last.jpg",
            Inline = true
        }.ToString());
        
        return new FileStreamResult(await cameraService.CreateFinalImageFromLasts(save), "image/jpeg");
    }
}