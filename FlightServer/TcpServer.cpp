#include "TcpServer.h"
#include "ClientHandler.h"
#include <QDebug>

TcpServer::TcpServer(QObject *parent) : QTcpServer(parent)
{
    m_dbHandler = new DbHandler(this);
    if (!m_dbHandler->connectDb("Mydb", "root", "cym060625")) {
        qFatal("数据库连接失败");
    }
}

bool TcpServer::startServer(quint16 port)
{
    if (listen(QHostAddress::Any, port)) {
        qInfo() << "服务器启动成功，端口：" << port;
        return true;
    } else {
        qCritical() << "服务器启动失败：" << errorString();
        return false;
    }
}

void TcpServer::incomingConnection(qintptr socketDescriptor)
{
    ClientHandler *handler = new ClientHandler(socketDescriptor, m_dbHandler, this);
    connect(handler, &ClientHandler::finished, handler, &ClientHandler::deleteLater);
    handler->start();
}
