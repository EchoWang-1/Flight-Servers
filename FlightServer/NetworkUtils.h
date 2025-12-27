#ifndef NETWORKUTILS_H
#define NETWORKUTILS_H

#include <QTcpSocket>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

class NetworkUtils
{
public:
    // 发送带长度前缀的 JSON
    static void sendJson(QTcpSocket *socket, const QJsonObject &json)
    {
        QByteArray jsonBytes = QJsonDocument(json).toJson(QJsonDocument::Compact);

        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_14);

        out << (quint32)0; // 占位长度
        out << jsonBytes;  // 写JSON数据

        out.device()->seek(0);
        out << (quint32)(block.size() - sizeof(quint32)); // 填入实际长度

        socket->write(block);
        socket->flush();
    }

    // 接收带长度前缀的 JSON
    static bool receiveJson(QTcpSocket *socket, QJsonObject &json)
    {
        static quint32 blockSize = 0;
        QDataStream in(socket);
        in.setVersion(QDataStream::Qt_5_14);

        if (blockSize == 0) {
            if (socket->bytesAvailable() < (int)sizeof(quint32))
                return false; // 长度不够
            in >> blockSize;
        }

        if (socket->bytesAvailable() < blockSize)
            return false; // 数据不够

        QByteArray jsonBytes;
        in >> jsonBytes;

        QJsonDocument doc = QJsonDocument::fromJson(jsonBytes);
        if (doc.isObject()) {
            json = doc.object();
        } else {
            qWarning() << "收到的不是有效 JSON";
            blockSize = 0; // 重置
            return false;
        }

        blockSize = 0; // 重置，准备接收下一个包
        return true;
    }
};

#endif // NETWORKUTILS_H
