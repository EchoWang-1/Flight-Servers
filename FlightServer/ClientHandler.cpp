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
    QJsonObject data ;
    QJsonObject resp;

    // 检查请求结构：有些请求数据在data字段，有些直接在根级别
    if (request.contains("data") && request["data"].isObject()) {
        data = request["data"].toObject();
    } else {
        // 如果没有data字段，尝试从根级别获取相关字段
        data = request;
        // 移除type字段，避免混淆
        data.remove("type");
    }

    qDebug() << "处理请求类型:" << type;
    qDebug() << "最终使用的数据:" << data;

    if (type == "login") {
        resp = handleLogin(data);
    }else if (type == "register") {
        resp = handleRegister(data);
    } else if (type == "check_phone") {
        resp = handleCheckPhone(data);
    } else if (type == "check_idcard") {
        resp = handleCheckIdCard(data);
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

    QString phone = data["phone"].toString();
    QString password = data["password"].toString();

    QJsonObject dbResp = m_dbHandler->verifyUser(phone, password);
    if (dbResp["code"].toInt() == 200) {
        resp["success"] = true;
        resp["data"] = dbResp["data"].toObject();
        // 添加调试信息
        qDebug() << "登录成功，返回前端的数据:" << resp["data"].toObject();
    } else {
        resp["success"] = false;
        resp["message"] = dbResp["msg"].toString();
    }
    return resp;
}


QJsonObject ClientHandler::handleRegister(const QJsonObject &data)
{
    QJsonObject resp;
    resp["type"] = "register_reply";

    // nickname 对应 username，不需要 realname
    QString username = data["nickname"].toString();  // nickname 作为 username
    QString password = data["password"].toString();
    QString phone = data["phone"].toString();
    QString idCard = data["id_card"].toString();  // 可选的身份证号

    QJsonObject dbResp = m_dbHandler->registerUser(username, password, phone, idCard);
    if (dbResp["code"].toInt() == 200) {
        resp["success"] = true;
        resp["data"] = dbResp["data"].toObject();
    } else {
        resp["success"] = false;
        resp["message"] = dbResp["msg"].toString();
    }
    return resp;
}

QJsonObject ClientHandler::handleCheckPhone(const QJsonObject &data)
{
    QJsonObject resp;
    resp["type"] = "check_phone_reply";

    QString phone = data["phone"].toString();
    QJsonObject dbResp = m_dbHandler->checkPhoneExists(phone);

    if (dbResp["code"].toInt() == 200) {
        resp["success"] = true;
        resp["data"] = QJsonObject{
            {"exists", dbResp["exists"].toBool()}
        };
    } else {
        resp["success"] = false;
        resp["message"] = dbResp["msg"].toString();
    }
    return resp;
}

QJsonObject ClientHandler::handleCheckIdCard(const QJsonObject &data)
{
    QJsonObject resp;
    resp["type"] = "check_idcard_reply";

    QString idCard = data["idCard"].toString();
    QJsonObject dbResp = m_dbHandler->checkIdCardExists(idCard);

    if (dbResp["code"].toInt() == 200) {
        resp["success"] = true;
        resp["data"] = QJsonObject{
            {"exists", dbResp["exists"].toBool()}
        };
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

    QString username = data["user_id"].toString();
    QString from = data["from_city"].toString();
    QString to = data["to_city"].toString();
    QString date = data["date"].toString();

    QJsonObject dbResp = m_dbHandler->getFlightList(username,from, to, date);
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
