using System.Reactive.Linq;
using Monitor.Extensions;
using Monitor.Models.HomeAssistant;
using Monitor.Services;
using NetDaemon.AppModel;
using NetDaemon.HassModel;

namespace Monitor.Apps;

[NetDaemonApp(Id = "serial_tx_app")]
public class SerialTxApp : AApp, IAsyncInitializable
{
    private readonly MqttEntity _txPayload;

    public SerialTxApp(IHaContext ha, ILogger<SerialTxApp> logger, EntitiesManagerService entitiesManagerService,
        SerialMessageService serialMessageService) : base(ha, logger, entitiesManagerService)
    {
        _txPayload = EntitiesManagerService.Entities.SerialTxPayload;

        _txPayload.StateChanges(logger)
            .Where(s => !string.IsNullOrWhiteSpace(s.Entity.State))
            .Select(s => s.Entity.State)
            .SubscribeAsync(async s =>
            {
                serialMessageService.SendCommand(s);
                
                entitiesManagerService.Update(_txPayload, string.Empty);
                await EntitiesManagerService.UpdateEntities();
            });
    }

    public async Task InitializeAsync(CancellationToken cancellationToken)
    {
        EntitiesManagerService.Update(_txPayload, string.Empty);
        await EntitiesManagerService.UpdateEntities();
    }
}