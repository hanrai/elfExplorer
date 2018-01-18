#ifndef ARCMODEL_H
#define ARCMODEL_H

#include <QAbstractListModel>
#include <QByteArray>

struct FilenameList
{
    char filename[12];
    quint32 size;
    quint32 offset;
};

class ArcModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit ArcModel(QObject *parent = 0);
    void init(QByteArray& data, quint32 count);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;

    enum ArcRoles {
        NameRole = Qt::UserRole + 1,
        OffsetRole,
        SizeRole
        };

private:
    QByteArray m_data;
    quint32 m_size;
    FilenameList *m_pFilenameList;
};

#endif // ARCMODEL_H
