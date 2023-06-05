using System.Globalization;
using System.Reactive.Linq;
using Monitor.Models;
using NetDaemon.HassModel;
using NetDaemon.HassModel.Entities;

namespace Monitor.Extensions;

public static class EntityExtensions
{
    public static IObservable<StateChange> FromUnavailableToAvailable(this Entity target, ILogger logger)
    {
        return target.StateChanges()
            .Where(s => s.Entity.IsUnavailable())
            .Do(s => s.Entity.LogState(logger))
            .WaitFor(target.StateChanges().Where(s => !s.Entity.IsUnavailable()), TimeSpan.FromDays(15))
            .Do(s => s.Entity.LogState(logger));
    }

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

    public static IObservable<StateChange> StateAllChanges(this Entity target, ILogger? logger = null)
    {
        return target.StateAllChanges()
            .Where(s => !s.Entity.IsUnavailable() && s.Old?.State != "unavailable")
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

    public static IObservable<Event> EventFor(this Entity entity, string eventType, IObservable<Event> events, ILogger? logger = null)
    {
        return events.Where(e =>
                    e.EventType == eventType
                    && e.DataElement?.GetProperty("entity_id").GetString() == entity.EntityId)
                .Do(_ => logger?.LogInformation("L'évènement {eventType} a été déclenché pour l'entité {entityId}", eventType, entity.EntityId))
            ;
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
            logger?.LogWarning("État non numérique pour {entityId} : {state}", entityId, state);
            
            return 0;
        }

        if (oldState == newState)
        {
            logger?.LogTrace("Pas de changement d'état pour {entityId} : {state}", entityId, state);

            return 0;
        }

        if (oldState < newState)
        {
            logger?.LogTrace("Augmentation de l'état pour {entityId} : {state}", entityId, state);
            
            return 1;
        }

        logger?.LogTrace("Diminution de l'état pour {entityId} : {state}", entityId, state);
        
        return -1;
    }

    public static void LogState(this Entity entityState, ILogger logger)
    {
        logger.LogInformation("L'entité {entityId} est dans l'état {state}", entityState.EntityId, entityState.State);
    }

    public static void LogState(this EntityState entityState, ILogger logger)
    {
        logger.LogInformation("L'entité {entityId} est dans l'état {state}", entityState.EntityId, entityState.State);
    }

    public static string FriendlyName(this Entity entity)
    {
        string? friendlyName = entity.WithAttributesAs<AttributesWithFriendlyNameAndDeviceClass>().Attributes?.FriendlyName;

        return string.IsNullOrWhiteSpace(friendlyName) ? 
            entity.EntityId : 
            friendlyName
                .Replace("contact", "")
                .Replace("occupancy", "")
            ;
    }

    public static string? DeviceClass(this Entity entity)
    {
        return entity.WithAttributesAs<AttributesWithFriendlyNameAndDeviceClass>().Attributes?.DeviceClass;
    }
    
    public static bool Debunce(this Entity entity, double seconds = 2, ILogger? logger = null)
    {
        return entity.TimeSinceLastChanged(logger)?.TotalSeconds >= seconds;
    }
    
    public static bool Debunce(this Entity entity, TimeSpan duration, ILogger? logger = null)
    {
        return entity.Debunce(duration.TotalSeconds, logger);
    }
    
    public static TimeSpan? TimeSinceLastChanged(this Entity entity, ILogger? logger = null)
    {
        DateTime? lastChanged = entity.GetLastChanged();
        
        if (lastChanged == null)
        {
            logger?.LogTrace("Pas de dernier changement pour l'entité {entity}", entity.EntityId);
            return null;
        }

        TimeSpan difference = (DateTime.Now - lastChanged.Value);
        double totalSecondsDifference = difference.TotalSeconds;
        
        logger?.LogTrace("Dernier changement de l'entité {entity} à {lastChanged} soit il y a {seconds} seconds", entity.EntityId, lastChanged, totalSecondsDifference);
        return difference;
    }
    
    public static DateTime? GetLastChanged(this Entity entity)
    {
        return entity.EntityState?.LastChanged;
    }
    
    public static DateTime? GetLastUpdated(this Entity entity)
    {
        return entity.EntityState?.LastUpdated;
    }

    public static string FrenchState(this Entity entity)
    {
        string state = entity.State ?? "inconnu";

        if (entity.EntityId.Contains("mouvement"))
        {
            return state.Replace("on", "détécté");
        }

        if (entity.EntityId.Contains("contact")) // porte / fenêtre
        {
            return state
                .Replace("on", "ouverte")
                .Replace("off", "fermée");
        }
        
        return state
            .Replace("on", "allumée") // all
            .Replace("off", "éteinte") // all
            .Replace("home", "détécté et présent") // device_tracker
            .Replace("not_home", "absent") // device_tracker
            .Replace("active", "démarré") // timer
            .Replace("idle", "terminé") // timer
            .Replace("closed", "ouvert") // cover
            .Replace("opened", "fermé"); // cover
    }

    public static bool IsUnavailable(this Entity target)
    {
        return target.IsState("unavailable") || target.IsState("unknown");
    }

    public static bool IsUnavailable(this EntityState state)
    {
        return state.IsState("unavailable") || state.IsState("unknown");
    }
    
    public static bool DateTimeValueIsPassed(this Entity entity, ILogger logger, TimeSpan? interval = null)
    {
        DateTime now = DateTime.UtcNow;
        
        logger.LogTrace("Est-ce que la date de {entity} est dans le passé ? {date} <= {now}", entity.EntityId, entity.State, now);

        DateTime? dateState = entity.GetDateTime();
        
        if (dateState == null)
        {
            logger.LogWarning("Est-ce que la date de {entity} est dans le passé ? Non car impossible de la parser : {date}", entity.EntityId, entity.State);

            return false;
        }

        if (interval.HasValue)
        {
            now = now.Subtract(interval.Value);
        }
        
        bool result = dateState <= now;
        
        logger.LogInformation("Est-ce que la date de {entity} est dans le passé ? {date} <= {now} : {result}", entity.EntityId, entity.State, now, result);

        return result;
    }
    
    public static DateTime? GetDateTime(this Entity entity, bool isLocal = false)
    {
        if (!string.IsNullOrWhiteSpace(entity.State) 
            && DateTime.TryParse(entity.State, DateTimeFormatInfo.CurrentInfo, 
                isLocal ? DateTimeStyles.AssumeLocal : DateTimeStyles.AssumeUniversal, 
                out DateTime dateState)
           )
        {
            return isLocal ? dateState.ToUniversalTime() : dateState;
        }

        return null;
    }

    public static string GetDomain(this Entity entity)
    {
        return entity.EntityId.Split(".").First();
    }

    public static string GetName(this Entity entity)
    {
        return entity.EntityId.Split(".").Last();
    }
}