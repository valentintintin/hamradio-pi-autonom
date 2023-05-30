namespace Monitor.WorkServices;

public abstract class AService
{
    protected readonly ILogger<AService> Logger;
    
    protected AService(ILogger<AService> logger)
    {
        Logger = logger;
    }
}