using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.FileProviders;
using Monitor.Apps;
using Monitor.Context;
using Monitor.Extensions;
using Monitor.Services;
using NetDaemon.AppModel;
using NetDaemon.Extensions.MqttEntityManager;
using NetDaemon.Extensions.Scheduler;
using NetDaemon.Runtime;

Console.WriteLine("Starting");

WebApplicationBuilder builder = WebApplication.CreateBuilder(args);

builder.Host
    .UseNetDaemonAppSettings()
    .UseNetDaemonRuntime()
    .UseNetDaemonMqttEntityManagement();

builder.Services
    .AddNetDaemonStateManager()
    .AddNetDaemonScheduler();

builder.Services.AddControllersWithViews();

builder.Services.AddDbContext<DataContext>(option =>
{
    option.UseSqlite(
        builder.Configuration.GetConnectionString("Default")!
        .Replace("{StoragePath}", builder.Configuration.GetValueOrThrow<string>("StoragePath"))
    );
});

builder.Services.AddNetDaemonApp<MpptApp>();
builder.Services.AddNetDaemonApp<MpptNightApp>();
builder.Services.AddNetDaemonApp<MpptWatchdogApp>();
builder.Services.AddNetDaemonApp<MpptLowBattery>();
builder.Services.AddNetDaemonApp<GpioApp>();
builder.Services.AddNetDaemonApp<SleepApp>();
builder.Services.AddNetDaemonApp<CameraCaptureApp>();
builder.Services.AddNetDaemonApp<SerialPortMessageApp>();
builder.Services.AddNetDaemonApp<LoraTxApp>();
builder.Services.AddNetDaemonApp<SerialTxApp>();

builder.Services.AddScoped<SerialMessageService>();
builder.Services.AddScoped<MonitorService>();
builder.Services.AddScoped<SystemService>();
builder.Services.AddScoped<CameraService>();
builder.Services.AddScoped<EntitiesManagerService>();

WebApplication app = builder.Build();

app.UseStaticFiles();

string storagePath = app.Configuration.GetValueOrThrow<string>("StoragePath");

Directory.CreateDirectory(storagePath);

app.UseStaticFiles(new StaticFileOptions
{
    FileProvider = new PhysicalFileProvider(storagePath),
    RequestPath = "/storage"
});

app.UseRouting();

app.UseAuthorization();

app.MapControllerRoute(
    name: "default",
    pattern: "{controller=Home}/{action=Index}/{id?}");

using IServiceScope scope = app.Services.GetService<IServiceScopeFactory>()!.CreateScope();
await scope.ServiceProvider.GetRequiredService<DataContext>().Database.MigrateAsync();
EntitiesManagerService entitiesManagerService = scope.ServiceProvider.GetRequiredService<EntitiesManagerService>();

await entitiesManagerService.Init();

IConfigurationSection configurationSection = app.Configuration.GetSection("System");
File.WriteAllText(configurationSection.GetValueOrThrow<string>("ShutdownFile"), "0");
File.Delete(configurationSection.GetValueOrThrow<string>("TimeFile"));

string storagePathCameras = Path.Combine(
    storagePath, 
    app.Configuration.GetSection("Cameras").GetValueOrThrow<string>("Path")
);
Directory.CreateDirectory($"{storagePathCameras}");
Directory.CreateDirectory($"{storagePathCameras}/final");

Console.WriteLine("Started");

await app.RunAsync();