using System.Reactive.Linq;
using Monitor.Extensions;
using Monitor.WorkServices;
using NetDaemon.Common;
using NetDaemon.HassModel;

namespace Monitor.Apps;

[NetDaemonApp(Id = "gpio_app")]
public class GpioApp : AApp
{
    public GpioApp(IHaContext ha, ILogger<GpioApp> logger, EntitiesManagerService entitiesManagerService,
        SerialMessageService serialMessageService) 
        : base(ha, logger, entitiesManagerService)
    {
        EntitiesManagerService.Entities.GpioWifi.StateChanges(logger)
            .Subscribe(s =>
            {
                serialMessageService.SetWifi(s.Entity.IsOn());
            });
        
        EntitiesManagerService.Entities.GpioNpr.StateChanges(logger)
            .Subscribe(s =>
            {
                serialMessageService.SetNpr(s.Entity.IsOn());
            });
    }
}