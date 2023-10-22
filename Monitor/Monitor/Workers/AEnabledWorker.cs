using System.Reactive.Linq;
using Monitor.Extensions;
using Monitor.Models;

namespace Monitor.Workers;

public abstract class AEnabledWorker : AWorker
{
    public readonly StringConfigEntity<bool> Enabled;
    
    protected AEnabledWorker(ILogger<AEnabledWorker> logger, IServiceProvider serviceProvider) : base(logger, serviceProvider)
    {
        Enabled = new StringConfigEntity<bool>($"worker/{GetType().Name.ToLower().Replace("app", "")}", true, true);
        EntitiesManagerService.Add(Enabled);
    }

    public override Task StartAsync(CancellationToken cancellationToken)
    {
        Observable.Interval(RetryDuration).Select(_ => Enabled.Value)
            .Merge(Enabled.ValueChanges().Select(v => v.value))
            .SubscribeAsync(async state =>
            {
                try
                {
                    await DoState(state);
                }
                catch (Exception e)
                {
                    Logger.LogCritical(e, "Crash of retry in {duration}", RetryDuration);
                }
            });

        return Task.CompletedTask;
    }

    private async Task DoState(bool state)
    {
        if (state)
        {
            if (Started)
            {
                return;
            }

            Logger.LogInformation("Starting {worker}", Enabled.Id);

            await Start();
            Started = true;

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

            Logger.LogInformation("Stopped {worker}", Enabled.Id);
        }
    }
}