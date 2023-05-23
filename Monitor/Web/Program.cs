using Microsoft.EntityFrameworkCore;
using Web.Context;
using Web.Services;
using Web.Workers;

WebApplicationBuilder builder = WebApplication.CreateBuilder(args);

builder.Services.AddControllersWithViews();
builder.Services.AddHostedService<SerialPortWorker>();
builder.Services.AddHostedService<WebcamWorker>();

builder.Services.AddDbContext<DataContext>(option =>
{
    option.UseSqlite(builder.Configuration.GetConnectionString("Default"));
});

builder.Services.AddSingleton<SerialMessageService>();
builder.Services.AddSingleton<FileDownloaderService>();

WebApplication app = builder.Build();

app.Services.CreateScope().ServiceProvider.GetRequiredService<DataContext>().Database.Migrate();

app.UseHttpsRedirection();
app.UseStaticFiles();

app.UseRouting();

app.UseAuthorization();

app.MapControllerRoute(
    name: "default",
    pattern: "{controller=Home}/{action=Index}/{id?}");

app.Run();