#ifndef DBHANDLER_H
#define DBHANDLER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>
#include "CommonDef.h"

class DbHandler : public QObject
{
    Q_OBJECT
public:
    explicit DbHandler(QObject *parent = nullptr);
    ~DbHandler();

    bool connectDb(const QString &dsn, const QString &user, const QString &password);
    bool isConnected();

    QJsonObject verifyUser(const QString &username, const QString &password);
    QJsonObject getUserInfo(const QString &username);
    QJsonObject changePassword(const QString &username, const QString &oldPwd, const QString &newPwd);

    QJsonObject getFlightList(const QString &username, const QString &fromCity, const QString &toCity, const QString &date);
    QJsonObject bookFlight(const QString &username, const QString &flightNum);

    QJsonObject getOrderListWithFlight(const QString &username);
    QJsonObject refundOrder(const QString &orderNum, const QString &username);

    QSqlDatabase getDb() const { return m_db; }

private:
    QSqlDatabase getThreadSafeDb();
    QSqlDatabase m_db;
    QString m_dsn;
    QString m_user;
    QString m_password;
};

#endif // DBHANDLER_H
