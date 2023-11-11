using System.Globalization;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using Monitor.Context;
using Monitor.Services;

namespace Monitor.Controllers;

[ApiController]
[Route("dev")]
public class DevController : AController
{
    private readonly DataContext _context;
    
    public DevController(ILogger<DevController> logger, IDbContextFactory<DataContext> contextFactory) : base(logger)
    {
        _context = contextFactory.CreateDbContext();
    }

    [HttpGet("test")]
    public object? Test()
    {
        return default;
    }

    [HttpGet("reset_database")]
    public void ResetDatabase()
    {
        Logger.LogTrace("Reset database");
        
        _context.LoRas.RemoveRange(_context.LoRas);
        _context.Systems.RemoveRange(_context.Systems);
        _context.Weathers.RemoveRange(_context.Weathers);
        _context.Mppts.RemoveRange(_context.Mppts);
        _context.SaveChanges();
        
        Logger.LogInformation("Reset database OK");
    }
}