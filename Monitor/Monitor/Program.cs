using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.FileProviders;
using Monitor.Context;
using Monitor.Services;
using Monitor.Workers;

Console.WriteLine("Starting");

WebApplicationBuilder builder = WebApplication.CreateBuilder(args);

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

IServiceProvider serviceProvider = app.Services.CreateScope().ServiceProvider;
serviceProvider.GetRequiredService<DataContext>().Database.Migrate();

Console.WriteLine("Started");

app.Run();