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

    [HttpGet("last.webp")]
    public ActionResult GetLast()
    {
        string? fileStream = _cameraService.GetFinalLast();

        if (string.IsNullOrWhiteSpace(fileStream))
        {
            return new NotFoundResult();
        }
        
        Response.Headers.Add("Content-Disposition", new ContentDisposition
        {
            FileName = "last.webp",
            Inline = true
        }.ToString());

        return new PhysicalFileResult(fileStream, "image/webp");
    }

    [HttpGet("current.webp")]
    public async Task<FileStreamResult> GetCurrent()
    {
        Response.Headers.Add("Content-Disposition", new ContentDisposition
        {
            FileName = "last.webp",
            Inline = true
        }.ToString());
        
        return new FileStreamResult(await _cameraService.CreateFinalImageFromLasts(false), "image/webp");
    }
}