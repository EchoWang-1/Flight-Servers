#include "DbHandler.h"
#include <QSqlError>
#include <QDebug>
#include <QUuid>
#include <QDateTime>
#include <QThread>

DbHandler::DbHandler(QObject *parent) : QObject(parent) {}

DbHandler::~DbHandler()
{
    if (m_db.isOpen())
        m_db.close();
}

bool DbHandler::connectDb(const QString &dsn, const QString &user, const QString &password)
{
    m_dsn = dsn;
    m_user = user;
    m_password = password;

    m_db = QSqlDatabase::addDatabase("QODBC", "main_connection");
    m_db.setDatabaseName(dsn);
    m_db.setUserName(user);
    m_db.setPassword(password);

    if (m_db.open()) {
        qDebug() << "数据库连接成功";
        return true;
    } else {
        qDebug() << "数据库连接失败：" << m_db.lastError().text();
        return false;
    }
}

bool DbHandler::isConnected()
{
    return m_db.isOpen();
}

QSqlDatabase DbHandler::getThreadSafeDb() {
    QString connectionName = QString("conn_%1").arg((quintptr)QThread::currentThreadId());

    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        if (db.isOpen()) {
            return db;
        } else {
            QSqlDatabase::removeDatabase(connectionName);
        }
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC", connectionName);
    db.setDatabaseName(m_dsn);
    db.setUserName(m_user);
    db.setPassword(m_password);

    if (!db.open()) {
        qWarning() << "线程数据库连接失败：" << db.lastError().text();
    }
    return db;
}

QJsonObject DbHandler::verifyUser(const QString &phone, const QString &password)
{
    QJsonObject resp;
    QSqlDatabase db = getThreadSafeDb();
    if (!db.isOpen()) {
        resp["code"] = 500;
        resp["msg"] = "数据库未连接";
        return resp;
    }

    QSqlQuery query(db);
    query.prepare("SELECT * FROM userdata WHERE phone = :phone");
    query.bindValue(":phone", phone);

    if (query.exec() && query.next()) {
        if (query.value("password").toString() == password) {
            resp["code"] = 200;
            // 确保返回完整的用户数据，包含 username 字段
            QJsonObject userData;
            userData["username"] = query.value("username").toString();
            userData["realname"] = query.value("realname").toString();
            userData["phone"] = query.value("phone").toString();
            userData["email"] = query.value("email").toString();
            userData["ID_card_number"] = query.value("ID_card_number").toString();

            resp["data"] = userData;
            resp["msg"] = "登录成功";

            qDebug() << "登录验证成功，返回用户数据:" << userData;
        } else {
            resp["code"] = 401;
            resp["msg"] = "密码错误";
        }
    } else {
        resp["code"] = 404;
        resp["msg"] = "用户不存在";
    }
    return resp;
}

QJsonObject DbHandler::registerUser(const QString &username, const QString &password,
                                    const QString &phone, const QString &idCard)
{
    QJsonObject resp;
    QSqlDatabase db = getThreadSafeDb();
    if (!db.isOpen()) {
        resp["code"] = 500;
        resp["msg"] = "数据库未连接";
        return resp;
    }

    // 检查用户名是否已存在
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM userdata WHERE username = :username");
    query.bindValue(":username", username);

    if (query.exec() && query.next() && query.value(0).toInt() > 0) {
        resp["code"] = 409;
        resp["msg"] = "用户名已存在";
        return resp;
    }

    // 检查手机号是否已存在
    query.prepare("SELECT COUNT(*) FROM userdata WHERE phone = :phone");
    query.bindValue(":phone", phone);

    if (query.exec() && query.next() && query.value(0).toInt() > 0) {
        resp["code"] = 409;
        resp["msg"] = "手机号已注册";
        return resp;
    }

    // 如果有身份证号，检查是否已存在
    if (!idCard.isEmpty()) {
        query.prepare("SELECT COUNT(*) FROM userdata WHERE ID_card_number = :idCard");
        query.bindValue(":idCard", idCard);

        if (query.exec() && query.next() && query.value(0).toInt() > 0) {
            resp["code"] = 409;
            resp["msg"] = "身份证号已注册";
            return resp;
        }
    }

    // 插入新用户（nickname 对应 username，realname 留空）
    query.prepare("INSERT INTO userdata (username, password, phone, ID_card_number, realname) "
                  "VALUES (:username, :password, :phone, :idCard, :realname)");
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    query.bindValue(":phone", phone);
    query.bindValue(":idCard", idCard.isEmpty() ? QVariant() : idCard);
    query.bindValue(":realname", "");  // realname 留空，因为前端没有提供

    if (query.exec()) {
        resp["code"] = 200;
        resp["msg"] = "注册成功";
        resp["data"] = QJsonObject{
            {"username", username},
            {"phone", phone}
        };
    } else {
        resp["code"] = 500;
        resp["msg"] = "注册失败: " + query.lastError().text();
    }

    return resp;
}

QJsonObject DbHandler::checkPhoneExists(const QString &phone)
{
    QJsonObject resp;
    QSqlDatabase db = getThreadSafeDb();
    if (!db.isOpen()) {
        resp["code"] = 500;
        resp["msg"] = "数据库未连接";
        return resp;
    }

    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM userdata WHERE phone = :phone");
    query.bindValue(":phone", phone);

    if (query.exec() && query.next()) {
        resp["code"] = 200;
        resp["exists"] = query.value(0).toInt() > 0;
    } else {
        resp["code"] = 500;
        resp["msg"] = "查询失败: " + query.lastError().text();
    }

    return resp;
}

QJsonObject DbHandler::checkIdCardExists(const QString &idCard)
{
    QJsonObject resp;
    QSqlDatabase db = getThreadSafeDb();
    if (!db.isOpen()) {
        resp["code"] = 500;
        resp["msg"] = "数据库未连接";
        return resp;
    }

    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM userdata WHERE ID_card_number = :idCard");
    query.bindValue(":idCard", idCard);

    if (query.exec() && query.next()) {
        resp["code"] = 200;
        resp["exists"] = query.value(0).toInt() > 0;
    } else {
        resp["code"] = 500;
        resp["msg"] = "查询失败: " + query.lastError().text();
    }

    return resp;
}

QJsonObject DbHandler::getUserInfo(const QString &username)
{
    QJsonObject resp;
    QSqlDatabase db = getThreadSafeDb();
    if (!db.isOpen()) {
        resp["code"] = 500;
        resp["msg"] = "数据库未连接";
        return resp;
    }

    QSqlQuery query(db);
    query.prepare("SELECT * FROM userdata WHERE username = :username");
    query.bindValue(":username", username);

    if (query.exec() && query.next()) {
        resp["code"] = 200;
        resp["data"] = QJsonObject{
            {"username", query.value("username").toString()},
            {"realname", query.value("realname").toString()},
            {"phone", query.value("phone").toString()},
            {"email", query.value("email").toString()}
        };
    } else {
        resp["code"] = 404;
        resp["msg"] = "用户不存在";
    }
    return resp;
}

QJsonObject DbHandler::changePassword(const QString &username, const QString &oldPwd, const QString &newPwd)
{
    QJsonObject resp;
    QSqlDatabase db = getThreadSafeDb();
    if (!db.isOpen()) {
        resp["code"] = 500;
        resp["msg"] = "数据库未连接";
        return resp;
    }
    qDebug() << "数据库修改密码 - 查询用户名:" << username;

    QSqlQuery query(db);
    query.prepare("SELECT password FROM userdata WHERE username = :username");
    query.bindValue(":username", username);

    if (query.exec() && query.next()) {
        if (query.value("password").toString() == oldPwd) {
            query.prepare("UPDATE userdata SET password = :newPwd WHERE username = :username");
            query.bindValue(":newPwd", newPwd);
            query.bindValue(":username", username);
            if (query.exec()) {
                resp["code"] = 200;
                resp["msg"] = "密码修改成功";
            } else {
                resp["code"] = 500;
                resp["msg"] = "密码修改失败";
            }
        } else {
            resp["code"] = 401;
            resp["msg"] = "原密码错误";
        }
    } else {
        resp["code"] = 404;
        resp["msg"] = "用户不存在";
        qDebug() << "用户不存在 - 查询的用户名:" << username;
    }
    return resp;
}

QJsonObject DbHandler::getFlightList(const QString &username, const QString &fromCity, const QString &toCity, const QString &date)
{
    QJsonObject resp;
    QSqlDatabase db = getThreadSafeDb();

    // 1. 检查数据库连接
    if (!db.isOpen()) {
        resp["code"] = 500;
        resp["msg"] = "数据库未连接";
        qDebug() << "Database not open!";
        return resp;
    }

    QSqlQuery query(db);

    // 2. 构造 SQL 语句
    // 使用子查询：在查询 flightdata 的同时，去 orders 表里统计该用户对该航班的有效订单数
    // 如果 count > 0，说明已预订
    QString sql = R"(
        SELECT f.*,
               (SELECT COUNT(*)
                FROM orders o
                WHERE o.flight_id = f.id
                  AND o.username = :username
                  AND (o.status = '待出行' OR o.status = '已完成')
               ) as is_booked_count
        FROM flightdata f
        WHERE 1=1
    )";

    // 3. 动态拼接筛选条件
    // 注意：这里的字段名要和数据库 flightdata 表一致，建议加上别名 f. 以防歧义
    if (!fromCity.isEmpty()) sql += " AND f.from_city = :from";
    if (!toCity.isEmpty())   sql += " AND f.to_city = :to";
    if (!date.isEmpty())     sql += " AND f.date = :date";

    query.prepare(sql);

    // 4. 绑定参数
    // 绑定当前查询的用户 (用于判断是否预订)
    query.bindValue(":username", username);
    //qDebug() << "------------------------------------------------";
    //qDebug() << "[查询预订状态] 用户名(username):" << username;
    //qDebug() << "------------------------------------------------";

    // 绑定筛选条件
    if (!fromCity.isEmpty()) query.bindValue(":from", fromCity);
    if (!toCity.isEmpty())   query.bindValue(":to", toCity);
    if (!date.isEmpty())     query.bindValue(":date", date);

    QJsonArray arr;

    // 5. 执行查询与数据映射
    if (query.exec()) {
        while (query.next()) {
            QJsonObject item;

            // --- 基础信息 ---
            item["flight_number"] = query.value("flight_num").toString();
            item["airline"] = query.value("airline").toString(); // 如果表里有就取消注释

            // --- 城市与机场 ---
            item["startCity"]    = query.value("from_city").toString();
            item["endCity"]      = query.value("to_city").toString();
            item["startAirport"] = query.value("from_airport").toString();
            item["endAirport"]   = query.value("to_airport").toString();

            // --- 时间信息 ---
            item["startDate"] = query.value("date").toString();
            item["endDate"]   = query.value("date").toString();
            item["startTime"] = query.value("depart_Time").toString();
            item["endTime"]   = query.value("arrive_Time").toString();

            // --- 状态与价格 ---
            int remaining = query.value("remaining").toInt();
            QString statusStr;
            if (remaining > 0) {
                statusStr = "有票";
            } else {
                statusStr = "售罄";
            }
            item["status"] = statusStr;
            item["price"] = query.value("price").toString();

            // 读取我们刚才在 SQL 里生成的临时列 is_booked_count
            int bookedCount = query.value("is_booked_count").toInt();

            // 如果计数大于0，说明有“已预订”或“已完成”的订单
            item["isBooked"] = (bookedCount > 0);

            //qDebug() << "------------------------------------------------";
            //qDebug() << "[是否预定调试] 收到预订请求:";
            //qDebug() << "[是否预定调试] 预定状态:" << item["isBooked"].toBool();
            //qDebug() << "------------------------------------------------";

            arr.append(item);
        }

        resp["code"] = 200;
        resp["data"] = arr;
        qDebug() << "Query success, found rows:" << arr.size();

    } else {
        resp["code"] = 500;
        resp["msg"] = "查询失败: " + query.lastError().text();
        qDebug() << "SQL Error:" << query.lastError().text();
    }

    return resp;
}

QJsonObject DbHandler::bookFlight(const QString &username, const QString &flightNum)
{
    // === 【新增调试打印】 ===
    qDebug() << "------------------------------------------------";
    qDebug() << "[预订调试] 收到预订请求:";
    qDebug() << "[预订调试] 用户名(username):" << username;
    qDebug() << "[预订调试] 航班号(flightNum):" << flightNum;
    qDebug() << "------------------------------------------------";


    QJsonObject resp;
    QSqlDatabase db = getThreadSafeDb();
    if (!db.isOpen()) {
        resp["code"] = 500;
        resp["msg"] = "数据库未连接";
        return resp;
    }

    QSqlQuery query(db);
    query.prepare("SELECT id, remaining, price FROM flightdata WHERE flight_num = :flight_num");
    query.bindValue(":flight_num", flightNum);
    if (query.exec() && query.next()) {
        int flightId = query.value("id").toInt();
        int remaining = query.value("remaining").toInt();
        QString price = query.value("price").toString();

        if (remaining <= 0) {
            resp["code"] = 400;
            resp["msg"] = "航班已售罄";
            return resp;
        }

        QString orderNum = QUuid::createUuid().toString().remove("{").remove("}").remove("-");
        query.prepare("INSERT INTO orders (order_num, username, flight_id, passenger, seat, price, status, create_time) VALUES (:order_num, :username, :flight_id, :passenger, :seat, :price, '待出行', NOW())");
        query.bindValue(":order_num", orderNum);
        query.bindValue(":username", username);
        query.bindValue(":flight_id", flightId);
        query.bindValue(":passenger", username);
        query.bindValue(":seat", QString("%1%2").arg(rand() % 30 + 1).arg(QChar('A' + (rand() % 6))));
        query.bindValue(":price", price);

        if (query.exec()) {
            query.prepare("UPDATE flightdata SET remaining = remaining - 1 WHERE id = :flight_id");
            query.bindValue(":flight_id", flightId);
            query.exec();

            resp["code"] = 200;
            resp["msg"] = "预订成功";
            resp["data"] = QJsonObject{{"order_num", orderNum}};
        } else {
            resp["code"] = 500;
            resp["msg"] = "订单创建失败";
        }
    } else {
        resp["code"] = 404;
        resp["msg"] = "航班不存在";
    }
    return resp;
}

QJsonObject DbHandler::getOrderListWithFlight(const QString &username)
{
    QJsonObject resp;
    QSqlDatabase db = getThreadSafeDb();
    if (!db.isOpen()) {
        resp["code"] = 500;
        resp["msg"] = "数据库未连接";
        return resp;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT o.order_num, o.status, o.price, o.create_time,
               f.flight_num, f.from_city, f.from_airport, f.to_city, f.to_airport,
               f.date, f.depart_time, f.arrive_time
        FROM orders o
        LEFT JOIN flightdata f ON o.flight_id = f.id
        WHERE o.username = :username
        ORDER BY o.create_time DESC
    )");
    query.bindValue(":username", username);

    QJsonArray arr;
    if (query.exec()) {
        while (query.next()) {
            arr.append(QJsonObject{
                {"order_num", query.value("order_num").toString()},
                {"flight_num", query.value("flight_num").toString()},
                {"from_city", query.value("from_city").toString()},
                {"from_airport", query.value("from_airport").toString()}, // 新增
                {"to_city", query.value("to_city").toString()},
                {"to_airport", query.value("to_airport").toString()},   // 新增
                {"date", query.value("date").toString()},
                {"depart_time", query.value("depart_time").toString()},
                {"arrive_time", query.value("arrive_time").toString()},
                {"status", query.value("status").toString()},
                {"price", query.value("price").toString()},
                {"create_time", query.value("create_time").toString()}
            });
        }
        resp["code"] = 200;
        resp["data"] = arr;
    } else {
        resp["code"] = 500;
        resp["msg"] = "订单查询失败";
    }
    return resp;
}

QJsonObject DbHandler::refundOrder(const QString &orderNum, const QString &username)
{
    QJsonObject resp;
    QSqlDatabase db = getThreadSafeDb();
    if (!db.isOpen()) {
        resp["code"] = 500;
        resp["msg"] = "数据库未连接";
        return resp;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT o.flight_id, o.status FROM orders o
        WHERE o.order_num = :order_num AND o.username = :username
    )");
    query.bindValue(":order_num", orderNum);
    query.bindValue(":username", username);

    if (query.exec() && query.next()) {
        QString status = query.value("status").toString();
        if (status == "已退票") {
            resp["code"] = 400;
            resp["msg"] = "订单已退票";
            return resp;
        }

        int flightId = query.value("flight_id").toInt();
        query.prepare("UPDATE orders SET status = '已退票' WHERE order_num = :order_num");
        query.bindValue(":order_num", orderNum);
        if (query.exec()) {
            query.prepare("UPDATE flightdata SET remaining = remaining + 1 WHERE id = :flight_id");
            query.bindValue(":flight_id", flightId);
            query.exec();

            resp["code"] = 200;
            resp["msg"] = "退票成功";
        } else {
            resp["code"] = 500;
            resp["msg"] = "退票失败";
        }
    } else {
        resp["code"] = 404;
        resp["msg"] = "订单不存在或不属于当前用户";
    }
    return resp;
}
