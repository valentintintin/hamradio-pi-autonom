using Monitor.Services;

namespace Monitor;

public class MqttConnect(EntitiesManagerService entitiesManagerService) : BackgroundService
{
    protected override async Task ExecuteAsync(CancellationToken cancellationToken)
    {
        await entitiesManagerService.ConnectMqtt();
    }
}