#pragma once

#include <QDebug>
#include <QMap>
#include <QString>
#include <QStringList>

#include "abstractlinknamespace.h"

#include <iostream>

class LinkConfiguration : public QObject
{
    Q_OBJECT

public:
    struct LinkConf {
        QStringList args;
        QString name;
        LinkType type = LinkType::None;

        /*
        bool operator == (const LinkConf& other)
        {
            return (name == other.name) \
                && (type == other.type) \
                && (args == other.args) \
                ;
        }

        operator QString() const
        {
            QString text(QStringLiteral("LinkConf{Name: %1, LinkType: %2, Arguments: (%3)}"));
            return text.arg(name, QString::number(type), args.join(":"));
        }*/
    };

    enum Error {
        MissingConfiguration, // This can be used in future for warnings and not real errors
        NoErrors = 0,
        NoType,
        InvalidType,
        NoArgs,
        InvalidArgsNumber,
        ArgsAreEmpty,
    };

    static const QMap<Error, QString> errorMap;

    LinkConfiguration(LinkType linkType = LinkType::None, QStringList args = QStringList(), QString name = QString())
        : _linkConf{args, name, linkType} {};
    LinkConfiguration(LinkConf& confLinkStructure)
        : _linkConf{confLinkStructure} {};
    LinkConfiguration(const LinkConfiguration& other, QObject* parent = nullptr)
        : QObject(parent)
        , _linkConf{other.configurationStruct()} {};
    ~LinkConfiguration() = default;

    const QStringList* args() const { return &_linkConf.args; };
    Q_INVOKABLE QStringList argsAsConst() const { return _linkConf.args; };

    Q_INVOKABLE QString name() const { return _linkConf.name; };
    void setName(QString name) { _linkConf.name = name; };

    Q_INVOKABLE AbstractLinkNamespace::LinkType type() const { return _linkConf.type; };
    void setType(LinkType type) { _linkConf.type = type; };

    const QString createConfString() const { return _linkConf.args.join(":"); };
    const QStringList createConfStringList() const { return _linkConf.args; };

    const QString createFullConfString() const;
    const QStringList createFullConfStringList() const;

    LinkConf configurationStruct() const { return _linkConf; };
    const LinkConf* configurationStructPtr() const { return &_linkConf; };

    Q_INVOKABLE bool isValid() const { return error() <= NoErrors; }

    Error error() const;

    static QString errorToString(Error error) { return errorMap[error]; }

    LinkConfiguration& operator = (const LinkConfiguration& other)
    {
        this->_linkConf = other.configurationStruct();
        return *this;
    }

    bool operator == (const LinkConfiguration& other)
    {
        auto linkconf = other.configurationStruct();
        return (this->_linkConf.name == linkconf.name) \
            && (this->_linkConf.type == linkconf.type) \
            && (this->_linkConf.args == linkconf.args) \
            ;
    }

    /*
    operator QString() const
    {
        QString text(QStringLiteral("LinkConfiguration{Name: %1, LinkType: %2, Arguments: (%3)}"));
        return text.arg(name(), QString::number(type()), args()->join(":"));
    }
    */

    QString serialPort() { return (_linkConf.args.size() ? _linkConf.args[0] : QString() ); }
    int serialBaudrate() { return (_linkConf.args.size() > 1 ? _linkConf.args[1].toInt() : 0 ); }

    QString udpHost() { return (_linkConf.args.size() ? _linkConf.args[0] : QString() ); }
    int udpPort() { return (_linkConf.args.size() > 1 ? _linkConf.args[1].toInt() : 0 ); }

private:
    LinkConf _linkConf;
};

QDebug operator<< (QDebug d, const LinkConfiguration& other);
QDataStream& operator<<(QDataStream &out, const LinkConfiguration linkConfiguration);
QDataStream& operator>>(QDataStream &in, LinkConfiguration &linkConfiguration);

Q_DECLARE_METATYPE(LinkConfiguration)
Q_DECLARE_METATYPE(LinkConfiguration::LinkConf)

struct LinkConfigurationRegisterStruct
{
    LinkConfigurationRegisterStruct()
    {
        std::cout << "Running...\n";
        static QBasicAtomicInt metatype_id = Q_BASIC_ATOMIC_INITIALIZER(0);
        if (const int id = metatype_id.loadAcquire()) {
            std::cout << "NOOPS\n";
            return;
        }
        std::cout << "YEP\n";
        const int newId = qRegisterMetaType<LinkConfiguration>("LinkConfiguration");
        metatype_id.storeRelease(newId);
        qRegisterMetaTypeStreamOperators<LinkConfiguration>("LinkConfiguration");
    }
};

namespace
{
    // Put in anonymous namespace, because this variable should not be accessed
    // from other translation units
    LinkConfigurationRegisterStruct foo;
}