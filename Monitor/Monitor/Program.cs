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
    .UseNetDaemonAppSettings()
    .UseNetDaemonRuntime()
    .UseNetDaemonMqttEntityManagement();

builder.Services.AddNetDaemonStateManager().AddNetDaemonScheduler();

builder.Services.AddControllersWithViews();

builder.Services.AddDbContext<DataContext>(option =>
{
    option.UseSqlite(builder.Configuration.GetConnectionString("Default"));
});

builder.Services.AddHostedService<SerialPortMessageWorker>();
builder.Services.AddHostedService<SerialPortLogWorker>();
// builder.Services.AddHostedService<WebcamWorker>();
builder.Services.AddHostedService<SystemInfoWorker>();

builder.Services.AddNetDaemonApp<MpptApp>();
builder.Services.AddNetDaemonApp<MpptNightApp>();
builder.Services.AddNetDaemonApp<MpptWatchdogApp>();
builder.Services.AddNetDaemonApp<MpptLowBattery>();
builder.Services.AddNetDaemonApp<GpioApp>();
builder.Services.AddNetDaemonApp<SleepApp>();

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

using IServiceScope scope = app.Services.GetService<IServiceScopeFactory>()!.CreateScope();
scope.ServiceProvider.GetRequiredService<DataContext>().Database.Migrate();
await scope.ServiceProvider.GetRequiredService<EntitiesManagerService>().Init();

Console.WriteLine("Started");

app.Run();