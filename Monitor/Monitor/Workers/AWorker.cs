using System.Reactive.Concurrency;
using System.Reactive.Linq;
using Monitor.Extensions;
using Monitor.Models;
using Monitor.Services;

namespace Monitor.Workers;

public abstract class AWorker : IHostedService, IAsyncDisposable
{
    public readonly MqttEntity<bool> Enabled;
    protected readonly ILogger<AWorker> Logger;
    protected readonly IServiceProvider Services;
    protected readonly IScheduler Scheduler;
    protected readonly EntitiesManagerService EntitiesManagerService;

    private readonly List<IDisposable> _disposables = new();
    
    private bool Started { get; set; } 

    protected AWorker(ILogger<AWorker> logger, IServiceProvider serviceProvider)
    {
        Logger = logger;
        Services = serviceProvider.CreateScope().ServiceProvider;
        Scheduler = Services.GetRequiredService<IScheduler>();
        EntitiesManagerService = Services.GetRequiredService<EntitiesManagerService>();
        
        Enabled = new MqttEntity<bool>($"worker/{GetType().Name.ToLower().Replace("app", "")}", true, true);
        EntitiesManagerService.Add(Enabled);
    }
    
    public Task StartAsync(CancellationToken cancellationToken)
    {
            AddDisposable(
                Observable.Return(false).Delay(TimeSpan.FromSeconds(5)).Select(_ => Enabled.Value)
                    .Merge(Enabled.ValueChanges().Select(v => v.value))
                    .SubscribeAsync(async state =>
            {
                Logger.LogDebug("{app} {state} {started}", Enabled.Id, state, Started);
                
                if (state)
                {
                    if (Started)
                    {
                        return;
                    }
                    
                    Logger.LogInformation("Starting {worker}", Enabled.Id);

                    Started = true;
                    await Start();
                
                    Logger.LogInformation("Started {worker}", Enabled.Id);
                }
                else
                {
                    if (!Started)
                    {
                        return;
                    }
                    
                    Logger.LogInformation("Stopping {worker}", Enabled.Id);

                    await Stop();
                    Started = false;
                
                    Logger.LogInformation("Stopped {worker}", Enabled.Id);
                }
            }));

            return Task.CompletedTask;
    }

    public async Task StopAsync(CancellationToken cancellationToken)
    {
        if (Started)
        {
            await Stop();
            Started = false;
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
        return Task.CompletedTask;
    }
}