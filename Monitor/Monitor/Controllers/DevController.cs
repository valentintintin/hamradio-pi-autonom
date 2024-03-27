using System.Globalization;
using AprsSharp.AprsParser;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using Monitor.Context;
using Monitor.Extensions;
using Monitor.Services;

namespace Monitor.Controllers;

[ApiController]
[Route("dev")]
public class DevController(
    ILogger<DevController> logger,
    IDbContextFactory<DataContext> contextFactory,
    SystemService systemService)
    : AController(logger)
{
    private readonly DataContext _context = contextFactory.CreateDbContext();

    [HttpGet("test")]
    public void Test()
    {
        Console.WriteLine("F4HVV-15>F4HVV,WIDE1-1:!/7V,.Ot#4I!!G Solar NPR+Lora digi+Cam |!!!i!\"!>!W!!|".ToAprsPacket().Sender);
    }

    [HttpGet("reset_database")]
    public void ResetDatabase()
    {
        Logger.LogDebug("Reset database");
        
        _context.LoRas.RemoveRange(_context.LoRas);
        _context.Systems.RemoveRange(_context.Systems);
        _context.Weathers.RemoveRange(_context.Weathers);
        _context.Mppts.RemoveRange(_context.Mppts);
        _context.SaveChanges();
        
        Logger.LogInformation("Reset database OK");
    }

    [HttpGet("shutdown")]
    public void Shutdown()
    {
        Logger.LogInformation("Shutdown asked by route");
        
        systemService.Shutdown().ConfigureAwait(false);
    }
}