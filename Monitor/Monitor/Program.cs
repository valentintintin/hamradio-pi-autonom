using System.Globalization;
using AntDesign;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.FileProviders;
using Monitor.Context;
using Monitor.Extensions;
using Monitor.Models.SerialMessages;
using Monitor.Services;
using Monitor.Workers;
using NetDaemon.Extensions.Scheduler;

Console.WriteLine("Starting");

WebApplicationBuilder builder = WebApplication.CreateBuilder(args);

builder.Services.AddRazorPages();
builder.Services.AddServerSideBlazor();
builder.Services.AddAntDesign();
builder.Services.AddResponseCompression();

builder.Services.AddNetDaemonScheduler();

builder.Services.Configure<HostOptions>(options =>
{
    options.BackgroundServiceExceptionBehavior = BackgroundServiceExceptionBehavior.Ignore;
});

builder.Services.AddDbContextFactory<DataContext>(option =>
{
    option.UseSqlite(
        builder.Configuration.GetConnectionString("Default")!
        .Replace("{StoragePath}", builder.Configuration.GetValueOrThrow<string>("StoragePath"))
    );
});

builder.Services.AddSingleton<MpptApp>();
builder.Services.AddSingleton<PowerNightApp>();
builder.Services.AddSingleton<WatchdogApp>();
builder.Services.AddSingleton<LowBatteryApp>();
builder.Services.AddSingleton<GpioApp>();
builder.Services.AddSingleton<CameraCaptureApp>();
builder.Services.AddSingleton<SerialPortMcuCommandsApp>();
builder.Services.AddSingleton<SystemInfoApp>();

builder.Services.AddHostedService<MpptApp>();
builder.Services.AddHostedService<PowerNightApp>();
builder.Services.AddHostedService<WatchdogApp>();
builder.Services.AddHostedService<LowBatteryApp>();
builder.Services.AddHostedService<GpioApp>();
builder.Services.AddHostedService<CameraCaptureApp>();
builder.Services.AddHostedService<SerialPortMcuCommandsApp>();
builder.Services.AddHostedService<SystemInfoApp>();

builder.Services.AddSingleton<MonitorService>();
builder.Services.AddSingleton<SerialMessageService>();
builder.Services.AddSingleton<SystemService>();
builder.Services.AddSingleton<CameraService>();
builder.Services.AddSingleton<EntitiesManagerService>();

WebApplication app = builder.Build();

app.UseResponseCompression();

app.UseStaticFiles();

app.UseRouting();

app.MapControllers();
app.MapBlazorHub();
app.MapFallbackToPage("/_Host");

string storagePath = app.Configuration.GetValueOrThrow<string>("StoragePath");

Directory.CreateDirectory(storagePath);

app.UseStaticFiles(new StaticFileOptions
{
    FileProvider = new PhysicalFileProvider(storagePath),
    RequestPath = "/storage"
});

using IServiceScope scope = app.Services.GetService<IServiceScopeFactory>()!.CreateScope();
await scope.ServiceProvider.GetRequiredService<DataContext>().Database.MigrateAsync();

LocaleProvider.DefaultLanguage = "fr-FR";
LocaleProvider.SetLocale(LocaleProvider.DefaultLanguage);
CultureInfo.DefaultThreadCurrentUICulture = CultureInfo.GetCultureInfo(LocaleProvider.DefaultLanguage);

Console.WriteLine("Started");

if (app.Configuration.GetSection("SerialPortMessage").GetValue<bool?>("Simulate") == true)
{
    MonitorService.State.Lora.LastRx.Add(("RX 1", DateTime.UtcNow));
    MonitorService.State.Lora.LastRx.Add(("RX 2", DateTime.UtcNow.AddHours(1)));
    MonitorService.State.Lora.LastTx.Add(("TX 1", DateTime.UtcNow));
    MonitorService.State.Lora.LastTx.Add(("TX 2", DateTime.UtcNow.AddMinutes(2)));
    MonitorService.State.Lora.LastTx.Add(("TX 3", DateTime.UtcNow.AddHours(2)));

    MonitorService.State.LastMessagesReceived.Add(new GpioData
    {
        Type = "gpio",
        Ldr = 123,
        Npr = true,
        Wifi = false
    });
    MonitorService.State.LastMessagesReceived.Add(new WeatherData
    {
        Type = "weaher",
        Temperature = 12.34f,
        Humidity = 56
    });
}

await app.RunAsync();

Console.WriteLine("Stopped");