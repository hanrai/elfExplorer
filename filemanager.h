#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QObject>
class QFile;
class ArcModel;
class QAbstractListModel;
class QUrl;

class FileManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString arcFilename READ getArcFilename NOTIFY arcFilenameChanged)
    Q_PROPERTY(QString mesScript READ getMesScript NOTIFY mesScriptChanged)
public:
    explicit FileManager(QObject *parent = 0);
    ~FileManager();
    QString getArcFilename() const;
    QString getMesScript() const;
    QAbstractListModel *getModel();

signals:
    void arcFilenameChanged(QString filename);
    void fileSystemError(QString reason);
    void mesScriptChanged(QString script);

public slots:
    void openArc(QString filename);
    void decodeFile(QString filename, quint32 offset, quint32 size);
    void saveMes(QString filename);

private:
    void replaceArcFileObject(QFile* file);
    QString getFilename(QString fullpath) const;
    quint32 decodeFilenames(QByteArray& dataBuffer);

private:
    QFile* m_arcFile;
    ArcModel* m_arcModel;
    QString m_mesScript;
};

#endif // FILEMANAGER_H
