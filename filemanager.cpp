#include "filemanager.h"
#include <QFile>
#include <QUrl>
#include "arcmodel.h"
#include "DecodeArcMes.h"
#include "mesdecoder.h"

FileManager::FileManager(QObject *parent) : QObject(parent)
{
    m_arcFile = 0;
    m_arcModel = new ArcModel(this);
}

FileManager::~FileManager()
{
    if(m_arcFile)
        delete m_arcFile;
}

QString FileManager::getArcFilename() const
{
    if(!m_arcFile)
        return QString();

    return getFilename(m_arcFile->fileName());
}

QString FileManager::getMesScript() const
{
    return m_mesScript;
}

QAbstractListModel *FileManager::getModel()
{
    return m_arcModel;
}

void FileManager::openArc(QString filename)
{
    if(filename.isEmpty())
        return;
    QUrl fn(filename);

    QFile* file = new QFile(fn.toLocalFile());
    if(!file->open(QIODevice::ReadOnly))
    {
        emit fileSystemError(file->errorString());
        delete file;
        return;
    }

    replaceArcFileObject(file);
    emit arcFilenameChanged(getArcFilename());
    QByteArray buffer;
    auto sz = decodeFilenames(buffer);

    m_arcModel->init(buffer, sz);

    file->close();
}

void FileManager::decodeFile(QString filename, quint32 offset, quint32 size)
{
    QStringList fn = filename.split(".");
    QString ext = fn.last();

    m_arcFile->open(QIODevice::ReadOnly);
    if(!m_arcFile->isOpen())
    {
        emit fileSystemError(m_arcFile->errorString());
        return;
    }
    m_arcFile->seek(offset);
    QByteArray src = m_arcFile->read(size);
    m_arcFile->close();

    QByteArray buffer(1000000,'\0');
    quint32 sz = DecodeArcMes(src.data(), buffer.data(), size);
    buffer.truncate(sz);
    if(!ext.compare("MES", Qt::CaseInsensitive))
    {
        auto decoder = MesDecoder(this);
        m_mesScript = decoder.Decode(buffer);
        emit mesScriptChanged(m_mesScript);
        return;
    }
}

void FileManager::saveMes(QString filename)
{
    QUrl fn = QUrl(filename);
    QString locFn = fn.toLocalFile();
    QFile saveFile(locFn);
    saveFile.open(QIODevice::WriteOnly|QIODevice::Text);
    if(!saveFile.isOpen())
    {
        emit fileSystemError(saveFile.errorString());
        return;
    }
    saveFile.write(m_mesScript.toLocal8Bit());
    saveFile.close();
}

void FileManager::replaceArcFileObject(QFile *file)
{
    if(m_arcFile)
        delete m_arcFile;

    m_arcFile = file;
}

QString FileManager::getFilename(QString fullpath) const
{
    QUrl fn = QUrl::fromLocalFile(fullpath);
    return fn.fileName();
}

quint32 FileManager::decodeFilenames(QByteArray &dataBuffer)
{
    quint32 fileCount;
    m_arcFile->read((char*)&fileCount, 4);
    Q_ASSERT(fileCount);

    dataBuffer = m_arcFile->read(fileCount*20);
    Q_ASSERT(dataBuffer.size() == fileCount*20);

    struct FilenameListStructure
    {
        quint32 filename[3];
        quint32 size;
        quint32 offset;
    };
    Q_ASSERT ( sizeof(FilenameListStructure)==20 );
    FilenameListStructure *pFilenameList;

    pFilenameList = (FilenameListStructure*) dataBuffer.data();

    for(quint32 i=0; i<fileCount; i++)
    {
        pFilenameList[i].filename[0] ^= 0x55555555;
        pFilenameList[i].filename[1] ^= 0x55555555;
        pFilenameList[i].filename[2] ^= 0x55555555;
        pFilenameList[i].size ^= 0xAA55AA55;
        pFilenameList[i].offset ^= 0x55AA55AA;
    }

    return fileCount;
}
