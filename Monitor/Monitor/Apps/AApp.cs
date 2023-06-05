using Monitor.WorkServices;
using NetDaemon.HassModel;

namespace Monitor.Apps;

public abstract class AApp
{
    protected readonly IHaContext Ha;
    protected readonly ILogger<AApp> Logger;
    protected readonly EntitiesManagerService EntitiesManagerService;

    protected AApp(IHaContext ha, ILogger<AApp> logger, EntitiesManagerService entitiesManagerService)
    {
        Ha = ha;
        Logger = logger;
        EntitiesManagerService = entitiesManagerService;
    }
}