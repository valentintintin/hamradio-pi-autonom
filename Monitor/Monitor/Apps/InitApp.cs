using NetDaemon.Common;
using NetDaemon.Extensions.MqttEntityManager;

namespace Monitor.Apps;

public class InitApp : NetDaemonAppBase
{
    private readonly IMqttEntityManager _entityManager;

    public InitApp(IMqttEntityManager entityManager)
    {
        _entityManager = entityManager;
    }
}