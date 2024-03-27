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
        
        var configurationSection = configuration.GetSection("AprsIs");
        var positionSection = configuration.GetSection("Position");
        
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
        Logger.LogDebug("Received APRS-IS Packet from {from} to {to} of type {type}", packet.Sender, packet.Destination, packet.InfoField.Type);
        
        if (packet.Path.Contains("?") || packet.Path.Contains("qAX") || packet.Path.Contains("RFONLY") ||
            packet.Path.Contains("NOGATE") || packet.Path.Contains("TCPXX"))
        {
            Logger.LogDebug("Packet from {from} to {to} has not the correct path {path} for RF", packet.Sender, packet.Destination, packet.Path);
            
            return;
        }

        if (!HasStationHeard())
        {
            Logger.LogInformation("No station heard since {duration}. So no TX", _durationHeard);
            return;
        }

        try
        {
            Packet packetToSend = new(packet.Sender, new List<string> { "TCPIP", _callsign }, packet.InfoField);
            var packetToSendTnc2 = packetToSend.EncodeTnc2();

            Logger.LogInformation("Packet ready to be send to RF: {packet}", packetToSendTnc2);

            _serialMessageService.SendLora(packetToSendTnc2);
        }
        catch (Exception e)
        {
            Logger.LogWarning(e, "Impossible to compute packet from {from} to {to} of type {type}", packet.Sender, packet.Destination, packet.InfoField.Type);
        }
    }

    private bool HasStationHeard()
    {
        if (!_durationHeard.HasValue)
        {
            return true;
        }
        
        var lastHeard = DateTime.UtcNow - _durationHeard.Value;
        return _context.LoRas.Any(e => !e.IsTx && e.CreatedAt >= lastHeard);
    }
}