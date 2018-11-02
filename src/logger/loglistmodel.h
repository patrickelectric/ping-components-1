#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QLoggingCategory>

#include "ringvector.h"

Q_DECLARE_LOGGING_CATEGORY(LOG_LIST_MODEL);

/**
 * @brief Model for qml log interface
 *
 */
class LogListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    /**
     * @brief Construct a new LogListModel
     *
     * @param parent
     */
    LogListModel(QObject* parent = nullptr);

    enum {
        Category,
        Color,
        Text,
        Time,
    };

    /**
     * @brief Return data
     *
     * @param index
     * @param role
     * @return QVariant
     */
    QVariant data(const QModelIndex& index, int role) const override;

    /**
     * @brief Get role names
     *
     * @return QHash<int, QByteArray>
     */
    QHash<int, QByteArray> roleNames() const override;

    /**
     * @brief Return the number of rows
     *
     * @param parent
     * @return int
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return _itemRing.size();
    }

    /**
     * @brief
     *
     * @param time
     * @param text
     * @param color
     * @param category index
     */
    void append(const QVariant time, const QVariant text, const QVariant color, QVariant category);

private:
    Q_DISABLE_COPY(LogListModel)
    QVector<int> _roles;

    QHash<int, QByteArray> _roleNames{
        {{LogListModel::Category}, {"category"}},
        {{LogListModel::Color}, {"foreground"}},
        {{LogListModel::Text}, {"display"}},
        {{LogListModel::Time}, {"time"}},
    };
    struct ItemPack {
        QVariant category;
        QVariant color;
        QVariant text;
        QVariant time;
    };
    // We are not changing things inside the const function, but accessing _itemRing fail to compile
    mutable RingVector<ItemPack> _itemRing;
};
