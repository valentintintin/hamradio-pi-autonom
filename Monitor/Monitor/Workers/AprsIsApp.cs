using System.Globalization;
using AprsSharp.AprsIsClient;
using AprsSharp.AprsParser;
using Monitor.Extensions;
using Monitor.Services;

namespace Monitor.Workers;

public class AprsIsApp : AEnabledWorker
{
    private readonly SerialMessageService _serialMessageService;
    private readonly AprsIsClient _aprsIsClient;
    private readonly string _callsign, _passcode, _server, _filter;
    
    public AprsIsApp(ILogger<AprsIsApp> logger, IServiceProvider serviceProvider,
        IConfiguration configuration) : base(logger, serviceProvider)
    {
        _serialMessageService = Services.GetRequiredService<SerialMessageService>();
        
        _aprsIsClient = new AprsIsClient(Services.GetRequiredService<ILogger<AprsIsClient>>());
        _aprsIsClient.ReceivedPacket += ComputeReceivedPacket;
        
        IConfigurationSection configurationSection = configuration.GetSection("AprsIs");
        IConfigurationSection positionSection = configuration.GetSection("Position");
        
        _callsign = configurationSection.GetValueOrThrow<string>("Callsign");
        _passcode = configurationSection.GetValueOrThrow<string>("Passcode");
        _server = configurationSection.GetValueOrThrow<string>("Server");
        _filter = $"r/{positionSection.GetValueOrThrow<double>("Latitude").ToString( CultureInfo.InvariantCulture)}/{positionSection.GetValueOrThrow<double>("Longitude").ToString(CultureInfo.InvariantCulture)}/{configurationSection.GetValueOrThrow<int>("RadiusKm")} -e/{_callsign} {configurationSection.GetValueOrThrow<string>("Filter")}\n";
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

        Packet packetToSend = new(_callsign, new List<string> { "TCPIP", _callsign }, packet.InfoField);
        string packetToSendTnc2 = packetToSend.EncodeTnc2();
        
        Logger.LogInformation("Packet {packet} ready to be send to RF", packetToSendTnc2);
        
        _serialMessageService.SendLora(packetToSendTnc2);
    }
}