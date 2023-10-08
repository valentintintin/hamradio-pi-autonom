using System.Diagnostics;
using System.Globalization;
using Microsoft.Extensions.Primitives;

namespace Monitor;

public class PerformanceMiddleware
{
    private readonly RequestDelegate _next;
    private readonly ILogger<PerformanceMiddleware> _logger;

    public PerformanceMiddleware(RequestDelegate requestDelegate, ILogger<PerformanceMiddleware> logger)
    {
        _next = requestDelegate;
        _logger = logger;
    }

    public Task Invoke(HttpContext httpContext)
    {
        StringValues headersAcceptLanguage = httpContext.Request.Headers.AcceptLanguage;
        string? firstLanguage = headersAcceptLanguage.FirstOrDefault()?.Split(',').FirstOrDefault();
        CultureInfo culture = CultureInfo.GetCultureInfo(firstLanguage ?? "fr");
        
        CultureInfo.CurrentCulture = culture;
        CultureInfo.DefaultThreadCurrentCulture = culture;
        
        Thread.CurrentThread.CurrentCulture = culture;
        Thread.CurrentThread.CurrentUICulture = culture;
        
        Stopwatch watch = new();
        watch.Start();

        Task nextTask = _next.Invoke(httpContext);
        nextTask.ContinueWith(t =>
        {
            long time = watch.ElapsedMilliseconds;
            string requestString = $"[{httpContext.Request.Method}]{httpContext.Request.Path}?{httpContext.Request.QueryString}";
            if (t.Status == TaskStatus.RanToCompletion)
            {
                _logger.LogInformation("{time}ms {requestString}", time, requestString);
            }
            else
            {
                _logger.LogWarning(t.Exception?.InnerException, "{time}ms [{status}] - {requestString}", time, t.Status, requestString);
            }
        });
        return nextTask;
    }
}