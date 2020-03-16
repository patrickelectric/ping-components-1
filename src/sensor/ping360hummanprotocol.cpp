#include "ping360hummanprotocol.h"

#include <QHostAddress>

QByteArray Ping360HummamProtocol::discoveryMessage() { return {"Discovery"}; }

Ping360DiscoveryResponse Ping360HummamProtocol::decodeDiscoveryResponse(const QString& response)
{
    auto lines = response.split(QStringLiteral("\r\n"));
    for (auto& line : lines) {
        line = line.trimmed();
    };


    if (lines.size() == Ping360DiscoveryResponse::numberOfLines) {
        return {.deviceName = lines[0], .organization = lines[1], .macAddress = lines[2], .ipAddress = lines[3]};
    }

    return {};
}

QByteArray Ping360HummamProtocol::ipAddressMessage(const QString& address)
{
    union Ipv4Union {
        uint8_t values[4];
        uint32_t ip;

        QString getPing360IpValueFromIndex(int index) const
        {
            return QString::number(values[index]).rightJustified(3, '0');
        }
    } ipv4Union;

    QHostAddress sensorAddress(address);
    bool ok = false;

    ipv4Union.ip = sensorAddress.toIPv4Address(&ok);
    return QStringLiteral("SetSS1IP %1.%2.%3.%4")
        .arg(ipv4Union.getPing360IpValueFromIndex(0), ipv4Union.getPing360IpValueFromIndex(1),
            ipv4Union.getPing360IpValueFromIndex(2), ipv4Union.getPing360IpValueFromIndex(3))
        .toLatin1();
}
