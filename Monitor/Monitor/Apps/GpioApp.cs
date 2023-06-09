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
        TimeSpan debunce = TimeSpan.FromSeconds(1);
        
        EntitiesManagerService.Entities.GpioWifi.StateChanges(logger)
            .Sample(debunce)
            .Subscribe(s =>
            {
                serialMessageService.SetWifi(s.New!.IsOn());
            });
        
        EntitiesManagerService.Entities.GpioNpr.StateChanges(logger)
            .Sample(debunce)
            .Subscribe(s =>
            {
                serialMessageService.SetNpr(s.New!.IsOn());
            });
    }
}