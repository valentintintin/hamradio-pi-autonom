namespace Monitor.Services;

public abstract class AService(ILogger<AService> logger)
{
    protected readonly ILogger<AService> Logger = logger;
}