using NetDaemon.HassModel.Entities;

namespace Monitor.Extensions;

public static class OnOffExtensions
{
    public static IObservable<StateChange> TurnedOn(this Entity target, ILogger? logger = null, bool force = false)
    {
        return target.StateChangesFor(logger: logger, predicate: s => s.Entity.IsOn(), force: force);
    }
    
    public static IObservable<StateChange> TurnedOff(this Entity target, ILogger? logger = null, bool force = false)
    {
        return target.StateChangesFor(logger: logger, predicate: s => s.Entity.IsOff(), force: force);
    }

    public static bool IsOn(this Entity entity, ILogger? logger = null)
    {
        return entity.IsState("on", logger);
    }
    
    public static bool IsOff(this Entity entity, ILogger? logger = null)
    {
        return entity.IsState("off", logger) || entity.IsState("0", logger);
    }
    
    public static bool IsOn(this EntityState entity, ILogger? logger = null)
    {
        return entity.IsState("on", logger);
    }
    
    public static bool IsOff(this EntityState entity, ILogger? logger = null)
    {
        return entity.IsState("off", logger) || entity.IsState("0", logger);
    }
}