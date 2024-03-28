using System.Globalization;
using System.Reactive.Linq;
using AprsSharp.AprsIsClient;
using AprsSharp.AprsParser;
using AprsSharp.Shared;
using GeoCoordinatePortable;
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
    private readonly string? _objectName, _objectComment;
    private readonly DataContext _context;
    private readonly Position? _objectPosition;
    private readonly TimeSpan? _durationHeard = TimeSpan.FromMinutes(30);
    private readonly TcpConnection _tcpConnection = new();

    public AprsIsApp(ILogger<AprsIsApp> logger, IServiceProvider serviceProvider,
        IConfiguration configuration, IDbContextFactory<DataContext> contextFactory) : base(logger, serviceProvider)
    {
        _serialMessageService = Services.GetRequiredService<SerialMessageService>();
        _context = contextFactory.CreateDbContext();

        _aprsIsClient = new AprsIsClient(Services.GetRequiredService<ILogger<AprsIsClient>>(), _tcpConnection);
        _aprsIsClient.ReceivedPacket += ComputeReceivedPacket;
        // _aprsIsClient.ReceivedTcpMessage += message => Logger.LogTrace(message);
        
        var configurationSection = configuration.GetSection("AprsIs");
        var positionSection = configuration.GetSection("Position");
        
        var latitude = positionSection.GetValueOrThrow<double>("Latitude");
        var longitude = positionSection.GetValueOrThrow<double>("Longitude");
        
        _callsign = configurationSection.GetValueOrThrow<string>("Callsign");
        _passcode = configurationSection.GetValueOrThrow<string>("Passcode");
        _server = configurationSection.GetValueOrThrow<string>("Server");
        _filter = $"r/{latitude.ToString( CultureInfo.InvariantCulture)}/{longitude.ToString(CultureInfo.InvariantCulture)}/{configurationSection.GetValueOrThrow<int>("RadiusKm")} -e/{_callsign} {configurationSection.GetValueOrThrow<string>("Filter")}\n";

        if (configurationSection.GetValue("AlwaysTx", true))
        {
            _durationHeard = null;
        }

        var beaconObjectSection = configurationSection.GetSection("BeaconObject");
        if (beaconObjectSection.GetValue("Enable", false))
        {
            _objectName = beaconObjectSection.GetValueOrThrow<string>("Name");
            _objectComment = beaconObjectSection.GetValue<string>("Comment");
            _objectPosition = new Position(new GeoCoordinate(latitude, longitude), 
                beaconObjectSection.GetValueOrThrow<char>("SymbolTable"),
                beaconObjectSection.GetValueOrThrow<char>("SymbolCode")
                );
            
            _aprsIsClient.ChangedState += state =>
            {
                if (state == ConnectionState.LoggedIn)
                {
                    SendBeaconObjectPacket();
                }
            };
            
            AddDisposable(Observable.Timer(TimeSpan.FromMinutes(30)).Subscribe(_ =>
            {
                SendBeaconObjectPacket();
            }));
        }
    }

    protected override Task Start()
    {
        AddDisposable(_aprsIsClient.Receive(_callsign, _passcode, _server, _filter));
        
        return Task.CompletedTask;
    }

    protected override async Task Stop()
    {
        if (_objectPosition != null)
        {
            SendBeaconObjectPacket(false);

            await Task.Delay(1500);
        }

        _aprsIsClient.Disconnect();

        await base.Stop();
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

    private void SendBeaconObjectPacket(bool alive = true)
    {
        var comment = alive ? $"{_objectComment} Up:{EntitiesManagerService.Entities.SystemUptime.Value}" : "Éteint";
        var packet = $"{_callsign}>TCPIP:){_objectName}{(alive ? '!' : '_')}{_objectPosition!.Encode()}{comment}";
        
        Logger.LogInformation("Send packet beacon object alive ? {alive} : {packet}", alive, packet);

        _tcpConnection.SendString(packet + "\n");
    }
}