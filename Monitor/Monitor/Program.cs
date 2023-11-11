using System.Globalization;
using AntDesign;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.FileProviders;
using Monitor;
using Monitor.Context;
using Monitor.Extensions;
using Monitor.Services;
using Monitor.Workers;
using NetDaemon.Extensions.Scheduler;

Console.WriteLine("Starting");

WebApplicationBuilder builder = WebApplication.CreateBuilder(args);

builder.Services.AddLogging(loggingBuilder =>
{
    loggingBuilder.AddSeq(builder.Configuration.GetSection("Logging").GetSection("Seq"));
});

builder.Services.AddRazorPages();
builder.Services.AddServerSideBlazor();
builder.Services.AddAntDesign();
builder.Services.AddResponseCompression();

builder.Services.AddNetDaemonScheduler();

builder.Services.Configure<HostOptions>(hostOptions =>
{
    hostOptions.BackgroundServiceExceptionBehavior = BackgroundServiceExceptionBehavior.Ignore;
});

builder.Services.AddDbContextFactory<DataContext>(option =>
{
    option.UseSqlite(
        builder.Configuration.GetConnectionString("Default")!
            .Replace("{StoragePath}", builder.Configuration.GetValueOrThrow<string>("StoragePath"))
    );
});

builder.Services.AddSingleton<MpptApp>();
builder.Services.AddSingleton<WatchdogApp>();
builder.Services.AddSingleton<BatteryApp>();
builder.Services.AddSingleton<GpioApp>();
builder.Services.AddSingleton<CameraCaptureApp>();
builder.Services.AddSingleton<SerialPortMcuCommandsApp>();
builder.Services.AddSingleton<AprsIsApp>();
builder.Services.AddSingleton<McuFeatureApp>();

builder.Services.AddHostedService(services => services.GetRequiredService<MpptApp>());
builder.Services.AddHostedService(services => services.GetRequiredService<GpioApp>());
builder.Services.AddHostedService(services => services.GetRequiredService<SerialPortMcuCommandsApp>());
builder.Services.AddHostedService(services => services.GetRequiredService<WatchdogApp>());
builder.Services.AddHostedService(services => services.GetRequiredService<BatteryApp>());
builder.Services.AddHostedService(services => services.GetRequiredService<CameraCaptureApp>());
builder.Services.AddHostedService(services => services.GetRequiredService<AprsIsApp>());
builder.Services.AddHostedService(services => services.GetRequiredService<McuFeatureApp>());

builder.Services.AddHostedService<MqttConnect>();

builder.Services.AddSingleton<MonitorService>();
builder.Services.AddSingleton<SerialMessageService>();
builder.Services.AddSingleton<SystemService>();
builder.Services.AddSingleton<CameraService>();
builder.Services.AddSingleton<EntitiesManagerService>();

WebApplication app = builder.Build();

app.UseMiddleware<PerformanceAndCultureMiddleware>();
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

using IServiceScope scope = app.Services.GetService<IServiceScopeFactory>()!.CreateAsyncScope();
await scope.ServiceProvider.GetRequiredService<DataContext>().Database.MigrateAsync();

LocaleProvider.DefaultLanguage = "fr-FR";
LocaleProvider.SetLocale(LocaleProvider.DefaultLanguage);

CultureInfo culture = CultureInfo.GetCultureInfo(LocaleProvider.DefaultLanguage);

CultureInfo.DefaultThreadCurrentCulture = culture;
CultureInfo.DefaultThreadCurrentUICulture = culture;
CultureInfo.CurrentCulture = culture;
Thread.CurrentThread.CurrentCulture = culture;
Thread.CurrentThread.CurrentUICulture = culture;

Console.WriteLine("Started");

await app.RunAsync();

Console.WriteLine("Stopped");


public class MqttConnect : BackgroundService
{
    private readonly EntitiesManagerService _entitiesManagerService;

    public MqttConnect(EntitiesManagerService entitiesManagerService)
    {
        _entitiesManagerService = entitiesManagerService;
    }

    protected override async Task ExecuteAsync(CancellationToken cancellationToken)
    {
        await _entitiesManagerService.ConnectMqtt();
    }
}