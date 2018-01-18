#include "arcmodel.h"

ArcModel::ArcModel(QObject *parent):QAbstractListModel(parent)
{
    m_pFilenameList = 0;
    m_size = 0;
}

void ArcModel::init(QByteArray &data, quint32 count)
{
    beginResetModel();
    m_data = data;
    m_size = count;
    m_pFilenameList = (FilenameList*)m_data.data();
    endResetModel();
}

int ArcModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_size;
}

QVariant ArcModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    if(index.row()>=m_size)
        return QVariant();

    auto i = index.row();

    if(role == Qt::DisplayRole || role == NameRole)
    {
        QString n = QString(m_pFilenameList[i].filename);
        n.truncate(12);
        return n;
    }
    else if(role == OffsetRole)
        return m_pFilenameList[i].offset;
    else if(role == SizeRole)
        return m_pFilenameList[i].size;
    else
        return QVariant();
}

QHash<int, QByteArray> ArcModel::roleNames() const
{
    QHash<int, QByteArray> roles;
          roles[NameRole] = "name";
          roles[SizeRole] = "size";
          roles[OffsetRole] = "offset";
          return roles;
}
