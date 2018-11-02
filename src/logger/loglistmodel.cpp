#include <QDebug>

#include "logger.h"
#include "loglistmodel.h"

PING_LOGGING_CATEGORY(LOG_LIST_MODEL, "ping.loglistmodel");

LogListModel::LogListModel(QObject* parent)
    : QAbstractListModel(parent)
{
    // Add all roles in _roles
    for(const auto key : _roleNames.keys()) {
        _roles.append(key);
    }

    //_itemRing.setAccessType(RingVector<ItemPack>::LIFO);
    _itemRing.fill({0, {}, "", ""}, 20);
    beginInsertRows(QModelIndex(), 0, _itemRing.size());
    insertRows(0, _itemRing.size());
    endInsertRows();
}

void LogListModel::append(const QVariant time, const QVariant text, const QVariant color, QVariant category)
{
    /*
    // Create a new row
    const int line = rowCount() + 1;
    auto modelIndex = index(line);
    qCDebug(LOG_LIST_MODEL) << __PRETTY_FUNCTION__;
    qCDebug(LOG_LIST_MODEL) << time << text << color << category << line << modelIndex;
    setData(modelIndex, category, LogListModel::Category);
    setData(modelIndex, color, LogListModel::Color);
    setData(modelIndex, text, LogListModel::Text);
    setData(modelIndex, time, LogListModel::Time);

    for(int i{0}; i < 20; i++) {
        qCDebug(LOG_LIST_MODEL) << index(i);
    }*/
    //qCDebug(LOG_LIST_MODEL) << "new";

    static int size = 0;
    if(_itemRing.size() <= size) {
        removeRow(_itemRing.size());
        //_size - _itemRing.size()
        //emit rowsRemoved(index(_size - _itemRing.size()), _size - _itemRing.size(), change.last, QPrivateSignal());
        //beginRemoveRows(QModelIndex(), _size - _itemRing.size(), 1);
        //endRemoveRows();
    }
    //beginInsertRows(QModelIndex(), _size, 1);
    _itemRing.append({category, color, text, time});
    insertRow(_itemRing.size());
    size++;

    for(int i{0}; i < 20; i++) {
        qCDebug(LOG_LIST_MODEL) << _itemRing[i].time;
    }
    //endInsertRows();
}

QVariant LogListModel::data(const QModelIndex& index, int role) const
{
    //qCDebug(LOG_LIST_MODEL) << "--------------------------------------------";

    const int indexRow = index.row();

    qCDebug(LOG_LIST_MODEL) << indexRow << index << role;

    switch(role) {
    case LogListModel::Color: {
        //qCDebug(LOG_LIST_MODEL) << "COLOR" << _itemRing[indexRow].color;
        return _itemRing[indexRow].color;
    }
    break;
    case LogListModel::Time: {
        //qCDebug(LOG_LIST_MODEL) << "TIME" << _itemRing[indexRow].time;
        return _itemRing[indexRow].time;
    }
    break;
    case LogListModel::Text:
        //qCDebug(LOG_LIST_MODEL) << "text" << _itemRing[indexRow].text;
        return _itemRing[indexRow].text;
    case LogListModel::Category: {
        //qCDebug(LOG_LIST_MODEL) << "category" << _itemRing[indexRow].category;
        return _itemRing[indexRow].category;
    }
    break;
    default:
        break;
    }

    qCDebug(LOG_LIST_MODEL) << "no valid";
    return {"No valid role or index."};
}

QHash<int, QByteArray> LogListModel::roleNames() const
{
    return _roleNames;
}
