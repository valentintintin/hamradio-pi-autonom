using Monitor.Services;

namespace Monitor.Workers;

public abstract class AWorker : IHostedService, IDisposable
{
    protected readonly ILogger<AWorker> Logger;
    protected readonly MonitorService MonitorService;
    protected readonly IConfigurationSection ConfigurationSection;
    protected readonly IServiceProvider ServiceProvider;

    protected AWorker(string configSectionName, ILogger<AWorker> logger, IConfiguration configuration, IServiceScopeFactory serviceScopeFactory)
    {
        Logger = logger;
        ServiceProvider = serviceScopeFactory.CreateScope().ServiceProvider;
        MonitorService = ServiceProvider.GetRequiredService<MonitorService>();
        ConfigurationSection = configuration.GetSection(configSectionName);
    }
    
    public async Task StartAsync(CancellationToken cancellationToken)
    {
        if (ConfigurationSection.GetValue<bool>("Enabled"))
        {
            await Start();
        }
    }

    public async Task StopAsync(CancellationToken cancellationToken)
    {
        if (ConfigurationSection.GetValue<bool>("Enabled"))
        {
            await Stop();
        }
    }

    public virtual void Dispose()
    {
    }

    protected abstract Task Start();

    protected virtual Task Stop()
    {
        return Task.CompletedTask;
    }
}