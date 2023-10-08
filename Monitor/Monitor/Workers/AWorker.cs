using System.Reactive.Concurrency;
using System.Reactive.Linq;
using Monitor.Extensions;
using Monitor.Models;
using Monitor.Services;

namespace Monitor.Workers;

public abstract class AWorker : IHostedService, IAsyncDisposable
{
    protected bool Started { get; set; } 

    protected readonly ILogger<AWorker> Logger;
    protected readonly IServiceProvider Services;
    protected readonly IScheduler Scheduler;
    protected readonly EntitiesManagerService EntitiesManagerService;
    private readonly List<IDisposable> _disposables = new();
    
    protected static readonly TimeSpan RetryDuration = TimeSpan.FromSeconds(5);

    protected AWorker(ILogger<AWorker> logger, IServiceProvider serviceProvider)
    {
        Logger = logger;
        Services = serviceProvider.CreateScope().ServiceProvider;
        Scheduler = Services.GetRequiredService<IScheduler>();
        EntitiesManagerService = Services.GetRequiredService<EntitiesManagerService>();
    }
    
    public virtual Task StartAsync(CancellationToken cancellationToken)
    {
        AddDisposable(Observable.Interval(RetryDuration).Where(_ => !Started).SubscribeAsync(async _ =>
        {
            try
            {
                await Start();
                Started = true;
            }
            catch (Exception e)
            {
                Logger.LogCritical(e, "Crash of worker, retry in {duration}", RetryDuration);
            }
        }));
        
        return Task.CompletedTask;
    }

    public async Task StopAsync(CancellationToken cancellationToken)
    {
        if (Started)
        {
            await Stop();
        }
    }

    public virtual async ValueTask DisposeAsync()
    {
        if (Started)
        {
            await Stop();
            Started = false;
        }
    }

    protected IDisposable AddDisposable(IDisposable disposable)
    {
        _disposables.Add(disposable);
        return disposable;
    }
    
    protected abstract Task Start();

    protected virtual Task Stop()
    {
        foreach (IDisposable disposable in _disposables)
        {
            disposable.Dispose();
        }

        Started = false;
        
        return Task.CompletedTask;
    }
}