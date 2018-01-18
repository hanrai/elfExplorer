#ifndef MESDECODER_H
#define MESDECODER_H

#include <QObject>
#include <QBuffer>

struct Node
{
    bool isNumber;
    qint32 number;
    QString str;
};

union ByteWorker
{
    quint8 byte[4];
    quint16 word[2];
    quint32 dword;
};

struct SubCommand
{
    quint8 type;
    quint8 buffer[23];
    Node command;
};

class MesDecoder : public QObject
{
    Q_OBJECT
public:
    explicit MesDecoder(QObject *parent = 0);
    QString Decode(QByteArray src);
signals:

public slots:
private:
    quint8 GetMes();
    Node Calculate();
    void PrepareSubCommands();
    void GameCommand();
    QString GetNumber(int number);
    QString GetDim16(quint32 index);
    QString GetDim16(Node& node);
    QString GetDim16(Node node, qint32 i);
    QString GetReg16(quint32 index);
    QString GetReg16(Node& node);
    QString GetReg16(Node node, qint32 i);
    QString GetFlags(quint32 index);
    QString GetFlags(Node& node);
    QString GetFlags(Node node, qint32 i);
    QString GetDim32(quint32 index);
    QString GetDim32(Node& node);
    QString GetDim32(Node node, qint32 i);
    QString GetReg32(quint32 index);
    QString GetReg32(Node& node);
    QString GetReg32(Node node, qint32 i);
    QString Readable(QString name, QStringList list, quint32 index);
    void Cursor();
    void Animation();
    void Saved();
    void Sound();
    void Palette();
    void Painting();
    void Move();
    void Cache();
    void PaletteMisc();
private:
    QString result;
    QBuffer buffer;
    SubCommand subCommands[30];
    quint32 subCommandIndex;
};

#endif // MESDECODER_H
