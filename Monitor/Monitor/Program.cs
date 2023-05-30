using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.FileProviders;
using Monitor.Apps;
using Monitor.Context;
using Monitor.WorkServices;
using Monitor.Workers;
using NetDaemon.AppModel;
using NetDaemon.Extensions.MqttEntityManager;
using NetDaemon.Extensions.Scheduler;
using NetDaemon.Runtime;

Console.WriteLine("Starting");

WebApplicationBuilder builder = WebApplication.CreateBuilder(args);

builder.Host
    .UseNetDaemonRuntime()
    .UseNetDaemonMqttEntityManagement();

builder.Services.AddNetDaemonApp<InitApp>();

builder.Services.AddNetDaemonStateManager().AddNetDaemonScheduler();

builder.Services.AddControllersWithViews();

builder.Services.AddDbContext<DataContext>(option =>
{
    option.UseSqlite(builder.Configuration.GetConnectionString("Default"));
});

builder.Services.AddHostedService<SerialPortMessageWorker>();
builder.Services.AddHostedService<SerialPortLogWorker>();
builder.Services.AddHostedService<WebcamWorker>();
builder.Services.AddHostedService<SystemInfoWorker>();
builder.Services.AddHostedService<MpptWorker>();

builder.Services.AddScoped<SerialMessageService>();
builder.Services.AddScoped<MonitorService>();
builder.Services.AddScoped<SystemService>();
builder.Services.AddScoped<CameraService>();
builder.Services.AddScoped<EntitiesManagerService>();

WebApplication app = builder.Build();

app.UseStaticFiles();

app.UseStaticFiles(new StaticFileOptions
{
    FileProvider = new PhysicalFileProvider(Path.Combine(Directory.GetCurrentDirectory(), "storage")),
    RequestPath = "/storage"
});

app.UseRouting();

app.UseAuthorization();

app.MapControllerRoute(
    name: "default",
    pattern: "{controller=Home}/{action=Index}/{id?}");

using (IServiceScope? scope = app.Services.GetService<IServiceScopeFactory>()?.CreateScope())
{
    scope?.ServiceProvider.GetRequiredService<DataContext>().Database.Migrate();

    IMqttEntityManager mqttEntityManager = scope?.ServiceProvider.GetRequiredService<IMqttEntityManager>();
    mqttEntityManager
        ?.CreateAsync("switch.gpio_wifi",
        new EntityCreationOptions
        {
            Name = "Wifi",
            DeviceClass = "outlet"
        }).ConfigureAwait(false);
    mqttEntityManager
        ?.CreateAsync("switch.gpio_npr",
        new EntityCreationOptions
        {
            Name = "NPR",
            DeviceClass = "outlet"
        }).ConfigureAwait(false);
    mqttEntityManager
        ?.CreateAsync("sensor.gpio_ldr_box",
        new EntityCreationOptions
        {
            Name = "LDR Box",
            DeviceClass = "illuminance"
        }).ConfigureAwait(false);
    
    mqttEntityManager
        ?.CreateAsync("binary_sensor.box_opened",
        new EntityCreationOptions
        {
            Name = "Box opened",
            DeviceClass = "lock"
        }).ConfigureAwait(false);
    mqttEntityManager
        ?.CreateAsync("sensor.mcu_status",
        new EntityCreationOptions
        {
            Name = "MCU Status",
            DeviceClass = "enum"
        }).ConfigureAwait(false);
    mqttEntityManager
        ?.CreateAsync("sensor.mcu_uptime",
        new EntityCreationOptions
        {
            Name = "MCU Uptime",
            DeviceClass = "timestamp"
        }).ConfigureAwait(false);
    
    mqttEntityManager
        ?.CreateAsync("sensor.weather_temperature",
        new EntityCreationOptions
        {
            Name = "Outside Temperature",
            DeviceClass = "temperature"
        }, new
        {
            state_class = "measurement"
        }).ConfigureAwait(false);
    mqttEntityManager
        ?.CreateAsync("sensor.weather_humidity",
        new EntityCreationOptions
        {
            Name = "Outside Humidity",
            DeviceClass = "humidity"
        }, new
        {
            state_class = "measurement"
        }).ConfigureAwait(false);
    
    mqttEntityManager
        ?.CreateAsync("sensor.mppt_battery_voltage",
        new EntityCreationOptions
        {
            Name = "MPPT Battery Voltage",
            DeviceClass = "voltage"
        }, new
        {
            unit_of_measurement = "mV",
            state_class = "measurement"
        }).ConfigureAwait(false);
    mqttEntityManager
        ?.CreateAsync("sensor.mppt_solar_voltage",
        new EntityCreationOptions
        {
            Name = "MPPT Solar Voltage",
            DeviceClass = "voltage"
        }, new
        {
            unit_of_measurement = "mV",
            state_class = "measurement"
        }).ConfigureAwait(false);
    mqttEntityManager
        ?.CreateAsync("sensor.mppt_battery_current",
        new EntityCreationOptions
        {
            Name = "MPPT Battery Current",
            DeviceClass = "current"
        }, new
        {
            unit_of_measurement = "mA",
            state_class = "measurement"
        }).ConfigureAwait(false);
    mqttEntityManager
        ?.CreateAsync("sensor.mppt_solar_current",
        new EntityCreationOptions
        {
            Name = "MPPT Solar Current",
            DeviceClass = "current"
        }, new
        {
            unit_of_measurement = "mA",
            state_class = "measurement"
        }).ConfigureAwait(false);
    mqttEntityManager
        ?.CreateAsync("sensor.mppt_charge_current",
        new EntityCreationOptions
        {
            Name = "MPPT Charge Current",
            DeviceClass = "current"
        }, new
        {
            unit_of_measurement = "mA",
            state_class = "measurement"
        }).ConfigureAwait(false);
    mqttEntityManager
        ?.CreateAsync("sensor.mppt_status",
        new EntityCreationOptions
        {
            Name = "MPPT Status",
        }).ConfigureAwait(false);
    mqttEntityManager
        ?.CreateAsync("binary_sensor.mppt_night",
        new EntityCreationOptions
        {
            Name = "MPPT Night",
            DeviceClass = "light"
        }).ConfigureAwait(false);
    mqttEntityManager
        ?.CreateAsync("binary_sensor.mppt_alert",
        new EntityCreationOptions
        {
            Name = "MPPT Alert",
            DeviceClass = "problem"
        }).ConfigureAwait(false);
    mqttEntityManager
        ?.CreateAsync("binary_sensor.mppt_power_enaled",
        new EntityCreationOptions
        {
            Name = "MPPT Power Enabled",
            DeviceClass = "power"
        }).ConfigureAwait(false);
    mqttEntityManager
        ?.CreateAsync("binary_sensor.mppt_watchdog_enaled",
        new EntityCreationOptions
        {
            Name = "MPPT Watchdog Enabled",
            DeviceClass = "running"
        }).ConfigureAwait(false);
    mqttEntityManager
        ?.CreateAsync("sensor.mppt_watchdog_counter",
        new EntityCreationOptions
        {
            Name = "MPPT Watchdog Counter",
            DeviceClass = "duration"
        }, new
        {
            unit_of_measurement = "s"
        }).ConfigureAwait(false);
    mqttEntityManager
        ?.CreateAsync("sensor.mppt_watchdog_power_off_time",
        new EntityCreationOptions
        {
            Name = "MPPT Watchdog Power Off time",
            DeviceClass = "duration"
        }, new
        {
            unit_of_measurement = "s"
        }).ConfigureAwait(false);
    mqttEntityManager
        ?.CreateAsync("sensor.mppt_power_off_voltage",
        new EntityCreationOptions
        {
            Name = "MPPT Power Off Voltage",
            DeviceClass = "voltage"
        }, new
        {
            unit_of_measurement = "mV"
        }).ConfigureAwait(false);
    mqttEntityManager
        ?.CreateAsync("sensor.mppt_power_on_voltage",
        new EntityCreationOptions
        {
            Name = "MPPT Power On Voltage",
            DeviceClass = "voltage"
        }, new
        {
            unit_of_measurement = "mV"
        }).ConfigureAwait(false);
    
    mqttEntityManager
        ?.CreateAsync("sensor.lora_tx_payload",
        new EntityCreationOptions
        {
            Name = "Lora TX Payload",
        }).ConfigureAwait(false);
    mqttEntityManager
        ?.CreateAsync("sensor.lora_rx_payload",
        new EntityCreationOptions
        {
            Name = "Lora RX Payload",
        }).ConfigureAwait(false);
    
    mqttEntityManager?.PrepareCommandSubscriptionAsync("switch.gpio_npr").Result
        .Subscribe(new Action<string>(async state =>
        {
            await mqttEntityManager.SetStateAsync("switch.gpio_npr", state).ConfigureAwait(false);
        }));
    
    mqttEntityManager?.PrepareCommandSubscriptionAsync("switch.gpio_wifi").Result
        .Subscribe(new Action<string>(async state =>
        {
            await mqttEntityManager.SetStateAsync("switch.gpio_wifi", state).ConfigureAwait(false);
        }));
}

Console.WriteLine("Started");

app.Run();