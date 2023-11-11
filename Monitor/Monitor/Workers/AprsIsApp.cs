using System.Globalization;
using System.Reactive.Linq;
using AprsSharp.AprsIsClient;
using AprsSharp.AprsParser;
using Microsoft.EntityFrameworkCore;
using Monitor.Context;
using Monitor.Extensions;
using Monitor.Services;

namespace Monitor.Workers;

public class AprsIsApp : AEnabledWorker
{
    private readonly SerialMessageService _serialMessageService;
    private readonly AprsIsClient _aprsIsClient;
    private readonly string _callsign, _passcode, _server, _filter;
    private readonly DataContext _context;
    private readonly TimeSpan? _durationHeard = TimeSpan.FromMinutes(30);

    public AprsIsApp(ILogger<AprsIsApp> logger, IServiceProvider serviceProvider,
        IConfiguration configuration, IDbContextFactory<DataContext> contextFactory) : base(logger, serviceProvider)
    {
        _serialMessageService = Services.GetRequiredService<SerialMessageService>();
        _context = contextFactory.CreateDbContext();

        _aprsIsClient = new AprsIsClient(Services.GetRequiredService<ILogger<AprsIsClient>>());
        _aprsIsClient.ReceivedPacket += ComputeReceivedPacket;
        
        IConfigurationSection configurationSection = configuration.GetSection("AprsIs");
        IConfigurationSection positionSection = configuration.GetSection("Position");
        
        _callsign = configurationSection.GetValueOrThrow<string>("Callsign");
        _passcode = configurationSection.GetValueOrThrow<string>("Passcode");
        _server = configurationSection.GetValueOrThrow<string>("Server");
        _filter = $"r/{positionSection.GetValueOrThrow<double>("Latitude").ToString( CultureInfo.InvariantCulture)}/{positionSection.GetValueOrThrow<double>("Longitude").ToString(CultureInfo.InvariantCulture)}/{configurationSection.GetValueOrThrow<int>("RadiusKm")} -e/{_callsign} {configurationSection.GetValueOrThrow<string>("Filter")}\n";

        if (configurationSection.GetValue("AlwaysTx", true))
        {
            _durationHeard = null;
        }
    }

    protected override Task Start()
    {
        AddDisposable(_aprsIsClient.Receive(_callsign, _passcode, _server, _filter));
        
        return Task.CompletedTask;
    }

    protected override Task Stop()
    {
        _aprsIsClient.Disconnect();

        return base.Stop();
    }

    private void ComputeReceivedPacket(Packet packet)
    {
        Logger.LogTrace("Received APRS-IS Packet {packet}", packet.EncodeTnc2());
        
        if (packet.Path.Contains("?") || packet.Path.Contains("qAX") || packet.Path.Contains("RFONLY") ||
            packet.Path.Contains("NOGATE") || packet.Path.Contains("TCPXX"))
        {
            Logger.LogTrace("Packet {packet} has not the correct path for RF", packet.EncodeTnc2());
            
            return;
        }

        if (!HasStationHeard())
        {
            Logger.LogInformation("No station heard since {duration}. So no TX", _durationHeard);
            return;
        }

        Packet packetToSend = new(_callsign, new List<string> { "TCPIP", _callsign }, packet.InfoField);
        string packetToSendTnc2 = packetToSend.EncodeTnc2();
        
        Logger.LogInformation("Packet {packet} ready to be send to RF", packetToSendTnc2);
        
        _serialMessageService.SendLora(packetToSendTnc2);
    }

    private bool HasStationHeard()
    {
        if (!_durationHeard.HasValue)
        {
            return true;
        }
        
        DateTime lastHeard = DateTime.UtcNow - _durationHeard.Value;
        return _context.LoRas.Any(e => !e.IsTx && e.CreatedAt >= lastHeard);
    }
}