#include "ping.h"

#include <functional>

#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QStringList>
#include <QThread>
#include <QUrl>

#include "hexvalidator.h"
#include "link/seriallink.h"
#include "networkmanager.h"
#include "networktool.h"
#include "notificationmanager.h"
#include "settingsmanager.h"

Q_LOGGING_CATEGORY(PING_PROTOCOL_PING, "ping.protocol.ping")

const int Ping::_pingMaxFrequency = 50;

Ping::Ping()
    :PingSensor()
    ,_points(_num_points, 0)
{
    setControlPanel({"qrc:/Ping1DControlPanel.qml"});
    setSensorVisualizer({"qrc:/Ping1DVisualizer.qml"});

    _periodicRequestTimer.setInterval(1000);
    connect(&_periodicRequestTimer, &QTimer::timeout, this, [this] {
        if(!link()->isWritable())
        {
            qCWarning(PING_PROTOCOL_PING) << "Can't write in this type of link.";
            _periodicRequestTimer.stop();
            return;
        }

        if(!link()->isOpen())
        {
            qCCritical(PING_PROTOCOL_PING) << "Can't write, port is not open!";
            _periodicRequestTimer.stop();
            return;
        }

        // Update lost messages count
        _lostMessages = 0;
        for(const auto& requestedId : requestedIds)
        {
            _lostMessages += requestedId.waiting;
        }
        emit lostMessagesUpdate();

        request(Ping1dId::PCB_TEMPERATURE);
        request(Ping1dId::PROCESSOR_TEMPERATURE);
        request(Ping1dId::VOLTAGE_5);
        request(Ping1dId::MODE_AUTO);
    });

    //connectLink(LinkType::Serial, {"/dev/ttyUSB2", "115200"});

    connect(this, &Sensor::connectionOpen, this, &Ping::startPreConfigurationProcess);

    // Wait for device id to load the correct settings
    connect(this, &Ping::srcIdUpdate, this, &Ping::setLastPingConfiguration);

    connect(this, &Ping::firmwareVersionMinorUpdate, this, [this] {
        // Wait for firmware information to be available before looking for new versions
        static bool once = false;
        if(!once)
        {
            once = true;
            NetworkTool::self()->checkNewFirmware("ping1d", std::bind(&Ping::checkNewFirmwareInGitHubPayload, this,
                                                  std::placeholders::_1));
        }
    });

    // TODO: @Patrick move it to devicemanager
    // Load last successful connection
    auto config = SettingsManager::self()->lastLinkConfiguration();
    qCDebug(PING_PROTOCOL_PING) << "Loading last configuration connection from settings:" << config;
}

void Ping::startPreConfigurationProcess()
{
    qCDebug(PING_PROTOCOL_PING) << "Start pre configuration task and requests.";
    if(!link()->isWritable()) {
        qCDebug(PING_PROTOCOL_PING) << "It's only possible to set last configuration when link is writable.";
        return;
    }

    SettingsManager::self()->lastLinkConfiguration(*link()->configuration());

    // Request device information
    request(Ping1dId::PING_ENABLE);
    request(Ping1dId::MODE_AUTO);
    request(Ping1dId::PROFILE);
    request(Ping1dId::FIRMWARE_VERSION);
    request(Ping1dId::DEVICE_ID);
    request(Ping1dId::SPEED_OF_SOUND);

    // Start periodic request timer
    _periodicRequestTimer.start();

    // Save configuration
    SettingsManager::self()->lastLinkConfiguration(*link()->configuration());
}

void Ping::loadLastPingConfigurationSettings()
{
    // Set default values
    for(const auto& key : _pingConfiguration.keys()) {
        _pingConfiguration[key].value = _pingConfiguration[key].defaultValue;
    }

    // Load settings for device using device id
    QVariant pingConfigurationVariant = SettingsManager::self()->getMapValue({"Ping", "PingConfiguration", QString(_srcId)});
    if(pingConfigurationVariant.type() != QVariant::Map) {
        qCWarning(PING_PROTOCOL_PING) << "No valid PingConfiguration in settings." << pingConfigurationVariant.type();
        return;
    }

    // Get the value of each configuration and set it on device
    auto map = pingConfigurationVariant.toMap();
    for(const auto& key : _pingConfiguration.keys()) {
        _pingConfiguration[key].set(map[key]);
    }
}

void Ping::updatePingConfigurationSettings()
{
    // Save all sensor configurations
    for(const auto& key : _pingConfiguration.keys()) {
        auto& dataStruct = _pingConfiguration[key];
        dataStruct.set(dataStruct.getClassValue());
        SettingsManager::self()->setMapValue({"Ping", "PingConfiguration", QString(_srcId), key}, dataStruct.value);
    }
}

void Ping::connectLink(LinkType connType, const QStringList& connString)
{
    Sensor::connectLink(LinkConfiguration{connType, connString});
    startPreConfigurationProcess();
}

void Ping::handleMessage(const ping_message& msg)
{
    qCDebug(PING_PROTOCOL_PING) << "Handling Message:" << msg.message_id();

    auto& requestedId = requestedIds[msg.message_id()];
    if(requestedId.waiting) {
        requestedId.waiting--;
        requestedId.ack++;
    }

    switch (msg.message_id()) {

    // This message is deprecated, it provides no added information because
    // the device id is already supplied in every message header
    case Ping1dId::DEVICE_ID: {
        ping1d_device_id m(msg);
        _srcId = m.source_device_id();

        emit srcIdUpdate();
    }
    break;

    case Ping1dId::DISTANCE: {
        ping1d_distance m(msg);
        _distance = m.distance();
        _confidence = m.confidence();
        _transmit_duration = m.transmit_duration();
        _ping_number = m.ping_number();
        _scan_start = m.scan_start();
        _scan_length = m.scan_length();
        _gain_setting = m.gain_setting();

        // TODO: change to distMsgUpdate() or similar
        emit distanceUpdate();
        emit pingNumberUpdate();
        emit confidenceUpdate();
        emit transmitDurationUpdate();
        emit scanStartUpdate();
        emit scanLengthUpdate();
        emit gainSettingUpdate();
    }
    break;

    case Ping1dId::DISTANCE_SIMPLE: {
        ping1d_distance_simple m(msg);
        _distance = m.distance();
        _confidence = m.confidence();

        emit distanceUpdate();
        emit confidenceUpdate();
    }
    break;

    case Ping1dId::PROFILE: {
        ping1d_profile m(msg);
        _distance = m.distance();
        _confidence = m.confidence();
        _transmit_duration = m.transmit_duration();
        _ping_number = m.ping_number();
        _scan_start = m.scan_start();
        _scan_length = m.scan_length();
        _gain_setting = m.gain_setting();
//        _num_points = m.profile_data_length(); // const for now
//        memcpy(_points.data(), m.data(), _num_points); // careful with constant

        // This is necessary to convert <uint8_t> to <int>
        // QProperty only supports vector<int>, otherwise, we could use memcpy, like the two lines above
        for (int i = 0; i < m.profile_data_length(); i++) {
            _points.replace(i, m.profile_data()[i] / 255.0);
        }

        // TODO: change to distMsgUpdate() or similar
        emit distanceUpdate();
        emit pingNumberUpdate();
        emit confidenceUpdate();
        emit transmitDurationUpdate();
        emit scanStartUpdate();
        emit scanLengthUpdate();
        emit gainSettingUpdate();
        emit pointsUpdate();
    }
    break;

    case Ping1dId::MODE_AUTO: {
        ping1d_mode_auto m(msg);
        if(_mode_auto != static_cast<bool>(m.mode_auto())) {
            _mode_auto = m.mode_auto();
            emit modeAutoUpdate();
        }
    }
    break;

    case  Ping1dId::PING_ENABLE: {
        ping1d_ping_enable m(msg);
        _ping_enable = m.ping_enabled();
        emit pingEnableUpdate();
    }
    break;

    case Ping1dId::PING_INTERVAL: {
        ping1d_ping_interval m(msg);
        _ping_interval = m.ping_interval();
        emit pingIntervalUpdate();
    }
    break;

    case Ping1dId::RANGE: {
        ping1d_range m(msg);
        _scan_start = m.scan_start();
        _scan_length = m.scan_length();
        emit scanLengthUpdate();
        emit scanStartUpdate();
    }
    break;

    case Ping1dId::GENERAL_INFO: {
        ping1d_general_info m(msg);
        _gain_setting = m.gain_setting();
        emit gainSettingUpdate();
    }
    break;

    case Ping1dId::GAIN_SETTING: {
        ping1d_gain_setting m(msg);
        _gain_setting = m.gain_setting();
        emit gainSettingUpdate();
    }
    break;

    case Ping1dId::SPEED_OF_SOUND: {
        ping1d_speed_of_sound m(msg);
        _speed_of_sound = m.speed_of_sound();
        emit speedOfSoundUpdate();
    }
    break;

    case Ping1dId::PROCESSOR_TEMPERATURE: {
        ping1d_processor_temperature m(msg);
        _processor_temperature = m.processor_temperature();
        emit processorTemperatureUpdate();
        break;
    }

    case Ping1dId::PCB_TEMPERATURE: {
        ping1d_pcb_temperature m(msg);
        _pcb_temperature = m.pcb_temperature();
        emit pcbTemperatureUpdate();
        break;
    }

    case Ping1dId::VOLTAGE_5: {
        ping1d_voltage_5 m(msg);
        _board_voltage = m.voltage_5(); // millivolts
        emit boardVoltageUpdate();
        break;
    }

    default:
        qWarning(PING_PROTOCOL_PING) << "UNHANDLED MESSAGE ID:" << msg.message_id();
        break;
    }

    emit parsedMsgsUpdate();
}

void Ping::firmwareUpdate(QString fileUrl, bool sendPingGotoBootloader, int baud, bool verify)
{
    if(fileUrl.contains("http")) {
        NetworkManager::self()->download(fileUrl, [this, sendPingGotoBootloader, baud, verify](const QString& path) {
            qCDebug(FLASH) << "Downloaded firmware:" << path;
            flash(path, sendPingGotoBootloader, baud, verify);
        });
    } else {
        flash(fileUrl, sendPingGotoBootloader, baud, verify);
    }
}

void Ping::flash(const QString& fileUrl, bool sendPingGotoBootloader, int baud, bool verify)
{
    flasher()->setState(Flasher::Idle);
    flasher()->setState(Flasher::StartingFlash);
    if(!HexValidator::isValidFile(fileUrl)) {
        auto errorMsg = QStringLiteral("File does not contain a valid Intel Hex format: %1").arg(fileUrl);
        qCWarning(PING_PROTOCOL_PING) << errorMsg;
        flasher()->setState(Flasher::Error, errorMsg);
        return;
    };

    SerialLink* serialLink = dynamic_cast<SerialLink*>(link());
    if (!serialLink) {
        auto errorMsg = QStringLiteral("It's only possible to flash via serial.");
        qCWarning(PING_PROTOCOL_PING) << errorMsg;
        flasher()->setState(Flasher::Error, errorMsg);
        return;
    }

    if(!link()->isOpen()) {
        auto errorMsg = QStringLiteral("Link is not open to do the flash procedure.");
        qCWarning(PING_PROTOCOL_PING) << errorMsg;
        flasher()->setState(Flasher::Error, errorMsg);
        return;
    }

    // Stop requests and messages from the sensor
    _periodicRequestTimer.stop();
    setPingFrequency(0);

    if (sendPingGotoBootloader) {
        qCDebug(PING_PROTOCOL_PING) << "Put it in bootloader mode.";
        ping1d_goto_bootloader m;
        m.updateChecksum();
        writeMessage(m);
    }

    // Wait for bytes to be written before finishing the connection
    while (serialLink->port()->bytesToWrite()) {
        qCDebug(PING_PROTOCOL_PING) << "Waiting for bytes to be written...";
        serialLink->port()->waitForBytesWritten();
        qCDebug(PING_PROTOCOL_PING) << "Done !";
    }

    qCDebug(PING_PROTOCOL_PING) << "Finish connection.";
    // TODO: Move thread delay to something more.. correct.
    QThread::msleep(1000);
    link()->finishConnection();

    QSerialPortInfo pInfo(serialLink->port()->portName());
    QString portLocation = pInfo.systemLocation();

    qCDebug(PING_PROTOCOL_PING) << "Save sensor configuration.";
    updatePingConfigurationSettings();

    qCDebug(PING_PROTOCOL_PING) << "Start flash.";
    QThread::msleep(1000);

    flasher()->setBaudRate(baud);
    flasher()->setFirmwarePath(fileUrl);
    flasher()->setLink(link()->configuration()[0]);
    flasher()->setVerify(verify);
    flasher()->flash();

    QThread::msleep(500);
    // Clear last configuration src ID to detect device as a new one
    connect(&_flasher, &Flasher::stateChanged, this, [this] {
        if(flasher()->state() == Flasher::States::FlashFinished)
        {
            QThread::msleep(500);
            // Clear last configuration src ID to detect device as a new one
            resetSensorLocalVariables();
            Sensor::connectLink(*link()->configuration());
        }

    });
}

void Ping::setLastPingConfiguration()
{
    if(_lastPingConfigurationSrcId == _srcId) {
        return;
    }
    _lastPingConfigurationSrcId = _srcId;
    if(!link()->isWritable()) {
        qCDebug(PING_PROTOCOL_PING) << "It's only possible to set last configuration when link is writable.";
        return;
    }

    // Load previous configuration with device id
    loadLastPingConfigurationSettings();

    // Print last configuration
    QString output = QStringLiteral("\nPingConfiguration {\n");
    for(const auto& key : _pingConfiguration.keys()) {
        output += QString("\t%1: %2\n").arg(key).arg(_pingConfiguration[key].value);
    }
    output += QStringLiteral("}");
    qCDebug(PING_PROTOCOL_PING).noquote() << output;


    // Set loaded configuration in device
    static QString debugMessage =
        QStringLiteral("Device configuration does not match. Waiting for (%1), got (%2) for %3");
    static auto lastPingConfigurationTimer = new QTimer();
    connect(lastPingConfigurationTimer, &QTimer::timeout, this, [this] {
        bool stopLastPingConfigurationTimer = true;
        for(const auto& key : _pingConfiguration.keys())
        {
            auto& dataStruct = _pingConfiguration[key];
            if(dataStruct.value != dataStruct.getClassValue()) {
                qCDebug(PING_PROTOCOL_PING) <<
                                            debugMessage.arg(dataStruct.value).arg(dataStruct.getClassValue()).arg(key);
                dataStruct.setClassValue(dataStruct.value);
                stopLastPingConfigurationTimer = false;
            }
            if(key.contains("automaticMode") && dataStruct.value) {
                qCDebug(PING_PROTOCOL_PING) << "Device was running with last configuration in auto mode.";
                // If it's running in automatic mode
                // no further configuration is necessary
                break;
            }
        }
        if(stopLastPingConfigurationTimer)
        {
            qCDebug(PING_PROTOCOL_PING) << "Last configuration done, timer will stop now.";
            lastPingConfigurationTimer->stop();
            do_continuous_start(ContinuousId::PROFILE);
        }
    });
    lastPingConfigurationTimer->start(500);
    lastPingConfigurationTimer->start();
}

void Ping::setPingFrequency(float pingFrequency)
{
    if (pingFrequency <= 0 || pingFrequency > _pingMaxFrequency) {
        qCWarning(PING_PROTOCOL_PING) << "Invalid frequency:" << pingFrequency;
        do_continuous_stop(ContinuousId::PROFILE);
    } else {
        int periodMilliseconds = 1000.0f / pingFrequency;
        qCDebug(PING_PROTOCOL_PING) << "Setting frequency(Hz) and period(ms):" << pingFrequency << periodMilliseconds;
        set_ping_interval(periodMilliseconds);
        do_continuous_start(ContinuousId::PROFILE);
    }
    qCDebug(PING_PROTOCOL_PING) << "Ping frequency" << pingFrequency;
}

void Ping::printSensorInformation() const
{
    qCDebug(PING_PROTOCOL_PING) << "Ping1D Status:";
    qCDebug(PING_PROTOCOL_PING) << "\t- board_voltage:" << _board_voltage;
    qCDebug(PING_PROTOCOL_PING) << "\t- pcb_temperature:" << _pcb_temperature;
    qCDebug(PING_PROTOCOL_PING) << "\t- processor_temperature:" << _processor_temperature;
    qCDebug(PING_PROTOCOL_PING) << "\t- ping_enable:" << _ping_enable;
    qCDebug(PING_PROTOCOL_PING) << "\t- distance:" << _distance;
    qCDebug(PING_PROTOCOL_PING) << "\t- confidence:" << _confidence;
    qCDebug(PING_PROTOCOL_PING) << "\t- transmit_duration:" << _transmit_duration;
    qCDebug(PING_PROTOCOL_PING) << "\t- ping_number:" << _ping_number;
    qCDebug(PING_PROTOCOL_PING) << "\t- start_mm:" << _scan_start;
    qCDebug(PING_PROTOCOL_PING) << "\t- length_mm:" << _scan_length;
    qCDebug(PING_PROTOCOL_PING) << "\t- gain_setting:" << _gain_setting;
    qCDebug(PING_PROTOCOL_PING) << "\t- mode_auto:" << _mode_auto;
    qCDebug(PING_PROTOCOL_PING) << "\t- ping_interval:" << _ping_interval;
    qCDebug(PING_PROTOCOL_PING) << "\t- points:" << QByteArray((const char*)_points.data(), _num_points).toHex(',');
}

void Ping::checkNewFirmwareInGitHubPayload(const QJsonDocument& jsonDocument)
{
    float lastVersionAvailable = 0.0;

    auto filesPayload = jsonDocument.array();
    for(const QJsonValue& filePayload : filesPayload) {
        qCDebug(PING_PROTOCOL_PING) << filePayload["name"].toString();

        // Get version from Ping_V(major).(patch)_115kb.hex where (major).(patch) is <version>
        static const QRegularExpression versionRegex(QStringLiteral(R"(Ping_V(?<version>\d+\.\d+)_115kb\.hex)"));
        auto filePayloadVersion = versionRegex.match(filePayload["name"].toString()).captured("version").toFloat();
        _firmwares[filePayload["name"].toString()] = filePayload["download_url"].toString();

        if(filePayloadVersion > lastVersionAvailable) {
            lastVersionAvailable = filePayloadVersion;
        }
    }
    emit firmwaresAvailableUpdate();

    auto sensorVersion = QString("%1.%2").arg(_firmware_version_major).arg(_firmware_version_minor).toFloat();
    static QString firmwareUpdateSteps{"https://github.com/bluerobotics/ping-viewer/wiki/firmware-update"};
    if(lastVersionAvailable > sensorVersion) {
        QString newVersionText =
            QStringLiteral("Firmware update for Ping available: %1<br>").arg(lastVersionAvailable) +
            QStringLiteral("<a href=\"%1\">Check firmware update steps here!</a>").arg(firmwareUpdateSteps);
        NotificationManager::self()->create(newVersionText, "green", StyleManager::infoIcon());
    }
}

void Ping::resetSensorLocalVariables()
{
    _ascii_text = QString();
    _nack_msg = QString();

    _srcId = 0;
    _dstId = 0;

    _device_type = 0;
    _device_model = 0;
    _device_revision = 0;
    _firmware_version_major = 0;
    _firmware_version_minor = 0;

    _distance = 0;
    _confidence = 0;
    _transmit_duration = 0;
    _ping_number = 0;
    _scan_start = 0;
    _scan_length = 0;
    _gain_setting = 0;
    _speed_of_sound = 0;

    _processor_temperature = 0;
    _pcb_temperature = 0;
    _board_voltage = 0;

    _ping_enable = false;
    _mode_auto = 0;
    _ping_interval = 0;

    _lastPingConfigurationSrcId = -1;
}

Ping::~Ping()
{
    updatePingConfigurationSettings();
}

QDebug operator<<(QDebug d, const Ping::messageStatus& other)
{
    return d << "waiting: " << other.waiting << ", ack: " << other.ack << ", nack: " << other.nack;
}
