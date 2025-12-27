#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QTcpServer>
#include "DbHandler.h"

class TcpServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit TcpServer(QObject *parent = nullptr);
    bool startServer(quint16 port);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    DbHandler *m_dbHandler;
};

#endif // TCPSERVER_H
