#include "ClientHandler.h"
#include "NetworkUtils.h"
#include <QJsonDocument>
#include <QDebug>

ClientHandler::ClientHandler(qintptr socketDescriptor, DbHandler *dbHandler, QObject *parent)
    : QThread(parent), m_socketDescriptor(socketDescriptor), m_dbHandler(dbHandler), m_socket(nullptr) {}

ClientHandler::~ClientHandler()
{
    if (m_socket) {
        m_socket->abort();
        m_socket->deleteLater();
    }
}

void ClientHandler::run()
{
    m_socket = new QTcpSocket();
    if (!m_socket->setSocketDescriptor(m_socketDescriptor)) {
        qWarning() << "Socket初始化失败：" << m_socket->errorString();
        return;
    }

    connect(m_socket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead, Qt::DirectConnection);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientHandler::onDisconnected, Qt::DirectConnection);

    exec();
}

void ClientHandler::onReadyRead()
{
    QJsonObject json;
    if (NetworkUtils::receiveJson(m_socket, json)) {
        qDebug() << "收到前端请求：" << json;
        processRequest(json);
    }
}

void ClientHandler::onDisconnected()
{
    m_socket->close();
    quit();
}

void ClientHandler::processRequest(const QJsonObject &request)
{
    QString type = request["type"].toString();
    QJsonObject data = request["data"].toObject();
    QJsonObject resp;

    if (type == "login") {
        resp = handleLogin(data);
    } else if (type == "get_user_info") {
        resp = handleGetUserInfo(data);
    } else if (type == "change_password") {
        resp = handleChangePassword(data);
    } else if (type == "get_flights") {
        resp = handleGetFlights(data);
    } else if (type == "book_flight") {
        resp = handleBookFlight(data);
    } else if (type == "get_user_orders") {
        resp = handleGetOrders(data);
    } else if (type == "refund_order") {
        resp = handleRefundOrder(data);
    } else {
        resp["type"] = "error";
        resp["success"] = false;
        resp["message"] = "未知请求类型";
    }

    NetworkUtils::sendJson(m_socket, resp);
}

// ===== 业务处理方法 =====
QJsonObject ClientHandler::handleLogin(const QJsonObject &data)
{
    QJsonObject resp;
    resp["type"] = "login_reply";

    QString username = data["username"].toString();
    QString password = data["password"].toString();

    QJsonObject dbResp = m_dbHandler->verifyUser(username, password);
    if (dbResp["code"].toInt() == 200) {
        resp["success"] = true;
        resp["data"] = dbResp["data"].toObject();
    } else {
        resp["success"] = false;
        resp["message"] = dbResp["msg"].toString();
    }
    return resp;
}

QJsonObject ClientHandler::handleGetUserInfo(const QJsonObject &data)
{
    QJsonObject resp;
    resp["type"] = "get_user_info_reply";

    QString username = data["user_id"].toString();
    QJsonObject dbResp = m_dbHandler->getUserInfo(username);
    if (dbResp["code"].toInt() == 200) {
        resp["success"] = true;
        resp["data"] = dbResp["data"].toObject();
    } else {
        resp["success"] = false;
        resp["message"] = dbResp["msg"].toString();
    }
    return resp;
}

QJsonObject ClientHandler::handleChangePassword(const QJsonObject &data)
{
    QJsonObject resp;
    resp["type"] = "change_password_reply";

    QString username = data["user_id"].toString();
    QString oldPwd = data["old_pwd"].toString();
    QString newPwd = data["new_pwd"].toString();

    QJsonObject dbResp = m_dbHandler->changePassword(username, oldPwd, newPwd);
    resp["success"] = (dbResp["code"].toInt() == 200);
    resp["message"] = dbResp["msg"].toString();
    return resp;
}

QJsonObject ClientHandler::handleGetFlights(const QJsonObject &data)
{
    QJsonObject resp;
    resp["type"] = "get_flights_reply";

    QString from = data["from_city"].toString();
    QString to = data["to_city"].toString();
    QString date = data["date"].toString();

    QJsonObject dbResp = m_dbHandler->getFlightList(from, to, date);
    if (dbResp["code"].toInt() == 200) {
        resp["success"] = true;
        resp["data"] = dbResp["data"].toArray();
    } else {
        resp["success"] = false;
        resp["message"] = dbResp["msg"].toString();
    }
    return resp;
}

QJsonObject ClientHandler::handleBookFlight(const QJsonObject &data)
{
    QJsonObject resp;
    resp["type"] = "book_flight_reply";

    QString username = data["user_id"].toString();
    QString flightNum = data["flight_number"].toString();

    QJsonObject dbResp = m_dbHandler->bookFlight(username, flightNum);
    resp["success"] = (dbResp["code"].toInt() == 200);
    resp["message"] = dbResp["msg"].toString();
    return resp;
}

QJsonObject ClientHandler::handleGetOrders(const QJsonObject &data)
{
    QJsonObject resp;
    resp["type"] = "get_user_orders_reply";

    QString username = data["user_id"].toString();
    QJsonObject dbResp = m_dbHandler->getOrderListWithFlight(username);
    if (dbResp["code"].toInt() == 200) {
        resp["success"] = true;
        resp["data"] = dbResp["data"].toArray();
    } else {
        resp["success"] = false;
        resp["message"] = dbResp["msg"].toString();
    }
    return resp;
}

QJsonObject ClientHandler::handleRefundOrder(const QJsonObject &data)
{
    QJsonObject resp;
    resp["type"] = "refund_order_reply";

    QString orderNum = data["order_id"].toString();
    QString username = data["user_id"].toString();

    QJsonObject dbResp = m_dbHandler->refundOrder(orderNum, username);
    resp["success"] = (dbResp["code"].toInt() == 200);
    resp["message"] = dbResp["msg"].toString();
    return resp;
}
