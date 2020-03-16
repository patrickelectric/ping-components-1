#pragma once

#include <QObject>

/**
 * @brief Helper struct for discovery message response
 * It is defined by the format:
 * Sensor Name\r\n
 * Organization\r\n
 * MAC Address:- 00-00-00-00-00-00\r\n
 * IP address:- 127.000.000.001\r\n
 *
 * E.g: "SONAR PING360  \r\nBlue Robotics\r\nMAC Address:- 54-10-EC-79-7D-C5\r\nIP Address:- 192.168.000.111\r\n"
 *
 */
struct Ping360DiscoveryResponse {
    QString deviceName;
    QString organization;
    QString macAddress;
    QString ipAddress;

    static const int numberOfLines = 5;
};

/**
 * @brief Helper class that provides an abstraction over the humman friendly protocol of Ping360
 * This is implemented following the "Ping360 Sonar Blue Robotics Communications Protocol v2.0" PDF.
 *
 */
class Ping360HummamProtocol : public QObject {
    Q_OBJECT
public:
    /**
     * @brief The Discovery message used by the host computer to find Ping360.
     *
     * @return QByteArray
     */
    static QByteArray discoveryMessage();

    /**
     * @brief Decode response from discovery message
     *  Check *Ping360DiscoveryResponse* definition for more information
     *
     * @param response
     * @return Ping360DiscoveryResponse
     */
    static Ping360DiscoveryResponse decodeDiscoveryResponse(const QString& response);

    /**
     * @brief Create the SetIpAddress message for Ping360.
     *
     * @param address Used to set the device address
     * @return QByteArray
     */
    static QByteArray ipAddressMessage(const QString& address);

private:
    Q_DISABLE_COPY(Ping360HummamProtocol)

    Ping360HummamProtocol() = delete;
};
