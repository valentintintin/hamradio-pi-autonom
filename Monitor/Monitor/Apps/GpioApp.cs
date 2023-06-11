using System.Reactive.Concurrency;
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
        SerialMessageService serialMessageService, IScheduler scheduler) 
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

        scheduler.ScheduleAsync(TimeSpan.FromSeconds(5), async (_, _) =>
        {
            Logger.LogInformation("Switch GPIOs");
            
            entitiesManagerService.Update(EntitiesManagerService.Entities.GpioWifi, EntitiesManagerService.Entities.WifiShouldTurnOn.IsOn(logger));
            entitiesManagerService.Update(EntitiesManagerService.Entities.GpioNpr, EntitiesManagerService.Entities.NprShouldTurnOn.IsOn(logger));

            await entitiesManagerService.UpdateEntities();
        });
    }
}