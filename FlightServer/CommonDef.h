#ifndef COMMONDEF_H
#define COMMONDEF_H

#include <QString>
#include <QJsonObject>

struct OrderData {
    QString order_num;
    QString flight_num;
    QString from_city;
    QString to_city;
    QString date;
    QString depart_time;
    QString arrive_time;
    QString status;
    QString price;
    QString create_time;
};

struct FlightData {
    QString flight_num;
    QString from_city;
    QString to_city;
    QString date;
    QString depart_time;
    QString arrive_time;
    QString price;
    int remaining;
};

struct UserData {
    QString username;
    QString realname;
    QString phone;
    QString email;
};

#endif // COMMONDEF_H
