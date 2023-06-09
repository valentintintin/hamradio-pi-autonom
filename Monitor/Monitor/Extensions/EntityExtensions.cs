using System.Reactive.Linq;
using NetDaemon.HassModel.Entities;

namespace Monitor.Extensions;

public static class EntityExtensions
{
    public static IObservable<StateChange> StateChanges(this Entity target, ILogger? logger = null, bool force = false)
    {
        return target.StateChanges()
            .Where(s => !s.Entity.IsUnavailable() && s.Old?.IsUnavailable() != true)
            .Where(s => force || s.Old?.State != s.New?.State)
            .Do(s =>
            {
                if (logger != null)
                {
                    s.Entity.LogState(logger);
                }
            });
    }
    
    public static IObservable<StateChange> StateChangesFor(this Entity target, Func<StateChange, bool> predicate, ILogger? logger = null, bool force = false)
    {
        return StateChanges(target, logger, force)
            .Where(predicate)
            .Do(s =>
            {
                if (logger != null)
                {
                    s.Entity.LogState(logger);
                }
            });
    }
    
    public static bool IsState(this Entity entity, string state, ILogger? logger = null)
    {
        bool isState = entity.State == state;

        if (logger != null && isState)
        {
            entity.LogState(logger);
        }

        return isState;
    }
    
    public static bool IsState(this EntityState entityState, string state, ILogger? logger = null)
    {
        bool isState = entityState.State == state;

        if (logger != null && isState)
        {
            entityState.LogState(logger);
        }

        return isState;
    }

    public static int DiffState(this StateChange stateChange, ILogger? logger = null)
    {
        string entityId = stateChange.Entity.EntityId;
        string? state = stateChange.Entity.State;
        
        if (stateChange.Old == null || stateChange.New == null)
        {
            return 0;
        }

        if (
            !int.TryParse(stateChange.Old.State, out int oldState)
            || !int.TryParse(stateChange.New.State, out int newState)
        )
        {
            logger?.LogWarning("State not numeric for {entityId} : {state}", entityId, state);
            
            return 0;
        }

        if (oldState == newState)
        {
            logger?.LogTrace("No change for {entityId} : {state}", entityId, state);

            return 0;
        }

        if (oldState < newState)
        {
            logger?.LogTrace("Rising state for {entityId} : {state}", entityId, state);
            
            return 1;
        }

        logger?.LogTrace("Decreasing state for {entityId} : {state}", entityId, state);
        
        return -1;
    }

    public static void LogState(this Entity entityState, ILogger logger)
    {
        logger.LogInformation("Entity {entityId} is in state {state}", entityState.EntityId, entityState.State);
    }

    public static void LogState(this EntityState entityState, ILogger logger)
    {
        logger.LogInformation("Entity {entityId} is in state {state}", entityState.EntityId, entityState.State);
    }

    public static bool IsUnavailable(this Entity target)
    {
        return target.IsState("unavailable") || target.IsState("unknown");
    }

    public static bool IsUnavailable(this EntityState state)
    {
        return state.IsState("unavailable") || state.IsState("unknown");
    }

    public static void TurnOn(this Entity entity, ILogger? logger = null)
    {
        entity.CallService("switch.turn_on", logger);
    }

    public static void TurnOff(this Entity entity, ILogger? logger = null)
    {
        entity.CallService("switch.turn_off", logger);
    }

    public static void Turn(this Entity entity, bool state, ILogger? logger = null)
    {
        if (state)
        {
            entity.TurnedOn(logger);
        }
        else
        {
            entity.TurnOff(logger);
        }
    }
}