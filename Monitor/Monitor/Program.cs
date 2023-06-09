using System.Reactive.Linq;
using System.Reflection;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.FileProviders;
using Monitor.Apps;
using Monitor.Context;
using Monitor.Extensions;
using Monitor.WorkServices;
using NetDaemon.AppModel;
using NetDaemon.Extensions.MqttEntityManager;
using NetDaemon.Extensions.Scheduler;
using NetDaemon.HassModel;
using NetDaemon.Runtime;

Console.WriteLine("Starting");

WebApplicationBuilder builder = WebApplication.CreateBuilder(args);

builder.Host
    .UseNetDaemonAppSettings()
    .UseNetDaemonRuntime()
    .UseNetDaemonMqttEntityManagement();

builder.Services
    .AddAppsFromAssembly(Assembly.GetExecutingAssembly())
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

IHaContext haContext = scope.ServiceProvider.GetRequiredService<IHaContext>();
haContext.Events.Take(1).Subscribe(_ =>
{
    EntitiesManagerService.Entities.CamerasStillImage = haContext.GetAllEntities()
        .Where(e => e.EntityId.EndsWith("_still_images")).ToList();
});

Console.WriteLine("Started");

await app.RunAsync();