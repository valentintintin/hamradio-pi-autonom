using System.Reactive.Linq;
using Monitor.Models.HomeAssistant;
using Monitor.WorkServices;
using NetDaemon.AppModel;
using NetDaemon.HassModel;

namespace Monitor.Apps;

[NetDaemonApp(Id = "lora_tx_app")]
public class LoraTxApp : AApp, IAsyncInitializable
{
    private readonly MqttEntity _txPayload;

    public LoraTxApp(IHaContext ha, ILogger<LoraTxApp> logger, EntitiesManagerService entitiesManagerService,
        SerialMessageService serialMessageService) : base(ha, logger, entitiesManagerService)
    {
        _txPayload = EntitiesManagerService.Entities.LoraTxPayload;

        _txPayload.StateChanges()
            .Where(s => !string.IsNullOrWhiteSpace(s.Entity.State))
            .Select(s => s.Entity.State)
            .SubscribeAsync(async s =>
            {
                serialMessageService.SendLora(s);
                
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