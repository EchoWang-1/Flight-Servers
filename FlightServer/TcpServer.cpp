#include "TcpServer.h"
#include "ClientHandler.h"
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>

TcpServer::TcpServer(QObject *parent) : QTcpServer(parent)
{
    m_dbHandler = new DbHandler(this);
    if (!m_dbHandler->connectDb("Mydb", "root", "123456")) {
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
    qInfo() << "新客户端连接：" << socketDescriptor;

    // 获取数据库名和表名
    QString dbName = getDatabaseName();
    QStringList tables = getTableNames();

    qInfo() << "当前数据库名：" << dbName;
    qInfo() << "当前数据库表：" << tables;

    // 创建客户端处理器
    ClientHandler *handler = new ClientHandler(socketDescriptor, m_dbHandler, this);
    connect(handler, &ClientHandler::finished, handler, &ClientHandler::deleteLater);
    handler->start();
}

QString TcpServer::getDatabaseName()
{
    QSqlQuery query(m_dbHandler->getDb());
    if (query.exec("SELECT DATABASE();")) {
        if (query.next()) {
            return query.value(0).toString();
        }
    }
    qWarning() << "获取数据库名失败：" << query.lastError().text();
    return QString();
}

QStringList TcpServer::getTableNames()
{
    QStringList tables;
    QSqlQuery query(m_dbHandler->getDb());
    if (query.exec("SHOW TABLES;")) {
        while (query.next()) {
            tables << query.value(0).toString();
        }
    } else {
        qWarning() << "获取表名失败：" << query.lastError().text();
    }
    return tables;
}
