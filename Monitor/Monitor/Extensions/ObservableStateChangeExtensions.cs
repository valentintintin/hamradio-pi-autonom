using System.Reactive.Concurrency;
using System.Reactive.Linq;
using NetDaemon.HassModel.Entities;

namespace Monitor.Extensions;

public static class ObservableStateChangeExtensions
{
    public static IObservable<StateChange> WaitFor(
        this IObservable<StateChange> observable,
        IObservable<StateChange> observableToWait,
        TimeSpan timeSpan,
        IScheduler? scheduler = null)
    {
        return observable
            .Select(_ => observableToWait)
            .Switch()
            .Buffer(timeSpan, 1, scheduler ?? (IScheduler)Scheduler.Default)
            .Select(s => s.FirstOrDefault())
            .Where(s => s != null)!;
    }
}