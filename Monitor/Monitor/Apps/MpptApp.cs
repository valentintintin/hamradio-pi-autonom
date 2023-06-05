using System.Reactive.Linq;
using Monitor.Extensions;
using Monitor.WorkServices;
using NetDaemon.AppModel;
using NetDaemon.HassModel;

namespace Monitor.Apps;

[NetDaemonApp(Id = "mppt_app")]
public class MpptApp : AApp
{
    public MpptApp(IHaContext ha, ILogger<MpptApp> logger, EntitiesManagerService entitiesManagerService,
        SystemService systemService, SerialMessageService serialMessageService) 
        : base(ha, logger, entitiesManagerService)
    {
        MqttEntity powerOnVoltageEntity = EntitiesManagerService.Entities.MpptPowerOnVoltage;
        MqttEntity powerOffVoltageEntity = EntitiesManagerService.Entities.MpptPowerOffVoltage;
        
        EntitiesManagerService.Entities.MpptAlert.TurnedOn(logger).Subscribe(_ =>
        {
            systemService.Shutdown();
        });

        powerOffVoltageEntity.StateChanges(logger)
            .Merge(powerOnVoltageEntity.StateChanges(logger))
            .Subscribe(_ =>
        {
            serialMessageService.SetPowerOnOffVoltage(
                powerOnVoltageEntity.State.ToInt(),
                powerOffVoltageEntity.State.ToInt()
            );
        });
    }
}