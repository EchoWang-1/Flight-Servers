#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QThread>
#include <QTcpSocket>
#include <QJsonObject>
#include "DbHandler.h"

class ClientHandler : public QThread
{
    Q_OBJECT
public:
    explicit ClientHandler(qintptr socketDescriptor, DbHandler *dbHandler, QObject *parent = nullptr);
    ~ClientHandler();

protected:
    void run() override;

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    void processRequest(const QJsonObject &request);
    QJsonObject handleLogin(const QJsonObject &data);
    QJsonObject handleRegister(const QJsonObject &data);
    QJsonObject handleCheckPhone(const QJsonObject &data);
    QJsonObject handleCheckIdCard(const QJsonObject &data);
    QJsonObject handleGetUserInfo(const QJsonObject &data);
    QJsonObject handleChangePassword(const QJsonObject &data);
    QJsonObject handleGetFlights(const QJsonObject &data);
    QJsonObject handleBookFlight(const QJsonObject &data);
    QJsonObject handleGetOrders(const QJsonObject &data);
    QJsonObject handleRefundOrder(const QJsonObject &data);
    QJsonObject handleAddPassenger(const QJsonObject &data);
    QJsonObject handleGetPassengers(const QJsonObject &data);
    QJsonObject handleUpdatePassenger(const QJsonObject &data);
    QJsonObject handleDeletePassenger(const QJsonObject &data);

    qintptr m_socketDescriptor;
    QTcpSocket *m_socket;
    DbHandler *m_dbHandler;
};

#endif // CLIENTHANDLER_H
