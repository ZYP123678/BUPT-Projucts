#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    server = new QTcpServer(this);

    if (!server->listen(QHostAddress::Any, 13520)) {
        qDebug() << "服务端无法开启";
        return;
    } else {
        qDebug() << "服务，启动!";
    }

    connect(server, &QTcpServer::newConnection, this, &Widget::onNewConnection);

    // 将所有用户的在线状态设为0
    QSqlQuery query;
    QString updateSql = "UPDATE user SET online = 0";
    if (!query.exec(updateSql)) {
        qDebug() << "更新在线状态失败：" << query.lastError();
        return;
    }

    qDebug() << "所有用户的在线状态已重置为离线";
}

void Widget::onNewConnection() {
    QTcpSocket *clientSocket = server->nextPendingConnection();
    socketList.append(clientSocket);

    QJsonObject json = {
        {"kind", SOCKET},
        {"socketID", socketID}
    };
    socketID++;

    clientSocket->write(QJsonDocument(json).toJson());
    clientSocket->flush();

    connect(clientSocket, &QTcpSocket::readyRead, this, &Widget::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &Widget::onClientDisconnected);
}

void Widget::onReadyRead() const {
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    QByteArray data = clientSocket->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    QJsonObject json = jsonDoc.object();

    qDebug() << "收到:" << json;

    QString kind = json["kind"].toString();
    if (kind == LOGIN) {
        Login(json);
    } else if (kind == REGISTER) {
        Register(json);
    } else if (kind == LOGOUT) {
        Log_out(json);
    } else if (kind == BAG) {
        Bag(json);
    } else if (kind == LOBBY) {
        Lobby(json);
    } else if (kind == WIN) {
        Win(json);
    } else if (kind == LOST) {
        Lost(json);
    } else if (kind == EXP) {
        Exp(json);
    } else if (kind == EXP_W) {
        Exp_2(json);
    } else if (kind == USERLIST) {
        Userlist(json);
    } else if(kind == BUY) {
        Buy(json);
    }
    else {
        qDebug() << "Unknown kind:" << kind;
    }

    if (json.contains("user_ID")) {
        int userID = json["user_ID"].toInt();
        UpdateUserMedals(userID);
    }
}

void Widget::UpdateUserMedals(int userID) const {
    // 查询用户的精灵总数
    QSqlQuery query;
    QString sql = QString("SELECT COUNT(*) FROM spirit WHERE user_ID = %1").arg(userID);
    if (!query.exec(sql) || !query.next()) {
        qDebug() << "查询精灵总数失败：" << query.lastError();
        return;
    }
    int totalSpirits = query.value(0).toInt();

    // 查询用户的15级以上的精灵总数
    sql = QString("SELECT COUNT(*) FROM spirit WHERE user_ID = %1 AND level = 15").arg(userID);
    if (!query.exec(sql) || !query.next()) {
        qDebug() << "查询15级以上精灵总数失败：" << query.lastError();
        return;
    }
    int highLevelSpirits = query.value(0).toInt();

    // 根据精灵总数更新 medal1
    int medal1 = 0;
    if (totalSpirits >= 16) {
        medal1 = 3;
    } else if (totalSpirits >= 8) {
        medal1 = 2;
    } else if (totalSpirits >= 4) {
        medal1 = 1;
    }

    // 根据15级以上的精灵总数更新 medal2
    int medal2 = 0;
    if (highLevelSpirits >= 5) {
        medal2 = 3;
    } else if (highLevelSpirits >= 3) {
        medal2 = 2;
    } else if (highLevelSpirits >= 1) {
        medal2 = 1;
    }

    // 更新用户表中的 medal1 和 medal2
    sql = QString("UPDATE user SET medal1 = %1, medal2 = %2 WHERE user_ID = %3").arg(medal1).arg(medal2).arg(userID);
    if (!query.exec(sql)) {
        qDebug() << "更新用户勋章信息失败：" << query.lastError();
    } else {
        qDebug() << "用户勋章信息更新成功！";
    }
}


void Widget::Buy(QJsonObject json) const {
    int money = json["money"].toInt();
    int drug = json["drug"].toInt();
    int userID = json["user_ID"].toInt();
    QSqlQuery query;
    QString sql = QString("UPDATE user SET  money = '%1', drug = '%2' WHERE user_ID = '%3';").arg(money).arg(drug).arg(userID);
    if (!query.exec(sql)) {
        qDebug() << "金钱数、药数更新失败" << query.lastError();
    }
}

void Widget::onClientDisconnected() {
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    socketList.removeAll(clientSocket);
    clientSocket->deleteLater();
}
//获胜增加经验和钱
void Widget::Exp_2(QJsonObject json) const
{
    int userID = json["user_ID"].toInt();
    int drug = json["drug"].toInt();
    QJsonObject spirit = json["spirit"].toObject();

    int spiritID = spirit["spirit_ID"].toInt();
    QString spiritName = spirit["spirit_name"].toString();
    QString evolution = spirit["evolution"].toString();
    int level = spirit["level"].toInt();
    int experience = spirit["experience"].toInt();
    int attack = spirit["attack"].toInt();
    int hp = spirit["hp"].toInt();
    int defense = spirit["defense"].toInt();
    double attackInterval = spirit["attackInterval"].toDouble(); // 注意：attackInterval 为 double

    // 确保数据库连接已经打开
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        qDebug() << "数据库未打开！";
        return;
    }

    QSqlQuery query(db);
    query.prepare("UPDATE spirit SET spirit_name = :spiritName, evolution = :evolution, level = :level, "
                  "experience = :experience, attack = :attack, hp = :hp, defense = :defense, attackInterval = :attackInterval "
                  "WHERE spirit_ID = :spiritID AND user_ID = :userID");

    query.bindValue(":spiritName", spiritName);
    query.bindValue(":evolution", evolution);
    query.bindValue(":level", level);
    query.bindValue(":experience", experience);
    query.bindValue(":attack", attack);
    query.bindValue(":hp", hp);
    query.bindValue(":defense", defense);
    query.bindValue(":attackInterval", attackInterval); // 使用双精度绑定
    query.bindValue(":spiritID", spiritID);
    query.bindValue(":userID", userID);

    // 调试输出
    qDebug() << "执行的 SQL 查询: " << query.executedQuery();
    qDebug() << "绑定的参数: " << query.boundValues();


    if (!query.exec()) {
        qDebug() << "更新精灵信息失败: " << query.lastError();
        // 适当处理错误
    } else {
        qDebug() << "精灵信息更新成功！";
    }
    QString sql = QString("UPDATE user SET victory_count = victory_count + 1, total_games = total_games + 1,money = money + 500,drug = '%1' WHERE user_ID = '%2';").arg(drug).arg(userID);
    if (!query.exec(sql)) {
        qDebug() << "更新胜利场数、总场数、金币和药数失败：" << query.lastError();
    } else {
        // 计算胜率并更新
        QString updateWinRatioQuery = QString("UPDATE user SET win_ratio = CAST(victory_count AS REAL) / total_games WHERE user_ID = '%1';").arg(userID);
        if (!query.exec(updateWinRatioQuery)) {
            qDebug() << "更新胜率失败：" << query.lastError();
        }
    }

}

void Widget::Userlist(QJsonObject json) const {
    int currentUserID = json["user_ID"].toInt();
    int client_socketID = json["socketID"].toInt();
    QSqlQuery query;

    // 查询所有用户（除了当前用户）
    QString selectUsersQuery = QString("SELECT * FROM user WHERE user_ID != %1").arg(currentUserID);
    QJsonArray usersArray;

    if (query.exec(selectUsersQuery)) {
        while (query.next()) {
            QJsonObject userInfo;
            int userID = query.value("user_ID").toInt();
            userInfo["user_ID"] = userID;
            userInfo["user_name"] = query.value("user_name").toString();
            userInfo["online"] = query.value("online").toInt();
            userInfo["pet_num"] = query.value("pet_num").toInt();
            userInfo["victory_count"] = query.value("victory_count").toInt();
            userInfo["total_games"] = query.value("total_games").toInt();
            userInfo["win_ratio"] = query.value("win_ratio").toDouble();
            userInfo["medal1"] = query.value("medal1").toInt();
            userInfo["medal2"] = query.value("medal2").toInt();

            // 查询用户的精灵信息
            QString selectSpiritsQuery = QString("SELECT * FROM spirit WHERE user_ID = %1").arg(userID);
            QSqlQuery spiritQuery;
            QJsonArray spiritsArray;

            if (spiritQuery.exec(selectSpiritsQuery)) {
                while (spiritQuery.next()) {
                    QJsonObject spirit;
                    spirit["spirit_ID"] = spiritQuery.value("spirit_ID").toInt();
                    spirit["user_ID"] = spiritQuery.value("user_ID").toInt();
                    spirit["spirit_name"] = spiritQuery.value("spirit_name").toString();
                    spirit["type"] = spiritQuery.value("type").toString();
                    spirit["level"] = spiritQuery.value("level").toInt();
                    spirit["experience"] = spiritQuery.value("experience").toInt();
                    spirit["attack"] = spiritQuery.value("attack").toInt();
                    spirit["hp"] = spiritQuery.value("hp").toInt();
                    spirit["defense"] = spiritQuery.value("defense").toInt();
                    spirit["attackInterval"] = spiritQuery.value("attackInterval").toDouble();
                    spirit["evolution"] = spiritQuery.value("evolution").toString();

                    spiritsArray.append(spirit);
                }
            } else {
                qDebug() << "查询精灵失败：" << spiritQuery.lastError().text();
            }

            userInfo["spirits"] = spiritsArray;
            usersArray.append(userInfo);
        }
    } else {
        qDebug() << "查询用户失败：" << query.lastError().text();
        return;
    }

    // 准备并发送 JSON 消息
    QJsonObject responseJson = {
        {"kind", USERLIST},
        {"socketID", client_socketID},
        {"users", usersArray}
    };

    SendMessage(responseJson);
}

//决斗场获胜得到新精灵
void Widget::Win(QJsonObject json) const{
    int userID = json["user_ID"].toInt();
    QJsonObject spirit = json["spirit"].toObject();

    QString spiritName = spirit["spirit_name"].toString();
    QString evolution = spirit["evolution"].toString();
    int level = spirit["level"].toInt();
    int experience = spirit["experience"].toInt();
    int attack = spirit["attack"].toInt();
    int hp = spirit["hp"].toInt();
    int defense = spirit["defense"].toInt();
    double attackInterval = spirit["attackInterval"].toDouble();

    // 根据精灵名字确定种类
    QString type;
    if (spiritName == "炎火猴" || spiritName == "仙人球") {
        type = "Attack";
    } else if (spiritName == "贝尔" || spiritName == "火炎贝") {
        type = "Defend";
    } else if (spiritName == "布布种子" || spiritName == "伊修") {
        type = "HP";
    } else if (spiritName == "比比鼠" || spiritName == "悠悠") {
        type = "Speed";
    } else {
        type = "未知"; // 默认类型为未知
    }

    // 确保数据库连接已经打开
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        qDebug() << "数据库未打开！";
        return;
    }

    // 插入新的精灵数据到 spirit 表
    QSqlQuery query(db);
    query.prepare("INSERT INTO spirit (user_ID, spirit_name, type, level, experience, attack, hp, defense, attackInterval, evolution) "
                  "VALUES (:userID, :spiritName, :type, :level, :experience, :attack, :hp, :defense, :attackInterval, :evolution)");

    query.bindValue(":userID", userID);
    query.bindValue(":spiritName", spiritName);
    query.bindValue(":type", type);
    query.bindValue(":level", level);
    query.bindValue(":experience", experience);
    query.bindValue(":attack", attack);
    query.bindValue(":hp", hp);
    query.bindValue(":defense", defense);
    query.bindValue(":attackInterval", attackInterval);
    query.bindValue(":evolution", evolution);

    if (!query.exec()) {
        qDebug() << "插入新精灵失败: " << query.lastError();
        // 适当处理错误
    } else {
        qDebug() << "新精灵插入成功！";
    }
    // 更新 user 表中的 pet_num
    query.prepare("UPDATE user SET pet_num = pet_num + 1 WHERE user_ID = :userID");
    query.bindValue(":userID", userID);
    if (!query.exec()) {
        qDebug() << "精灵数目更新失败: " << query.lastError();
        // 适当处理错误
    } else {
        qDebug() << "新精灵插入成功！";
    }
}
//角斗场失败失去精灵
void Widget::Lost(QJsonObject json) const{
    int spiritID = json["spirit_ID"].toInt();
    int userID = json["user_ID"].toInt();
    qDebug()<<spiritID<<userID<<Qt::endl;
    // 创建查询对象
    QSqlQuery query;

    // 准备删除语句
    QString sql = QString("DELETE FROM spirit WHERE spirit_ID = '%1' AND user_ID = '%2';").arg(spiritID).arg(userID);
    if (!query.exec(sql)) {
        qDebug() << "删除精灵失败" << query.lastError();
    } else {
        qDebug() << "删除精灵成功";
        sql = QString("UPDATE user SET pet_num = pet_num - 1 WHERE user_ID = '%1';").arg(userID);
        if (!query.exec(sql)) {
            qDebug() << "精灵数目减少失败" << query.lastError();
        }
    }


    sql = QString("SELECT pet_num FROM user WHERE user_ID = %1").arg(userID);
    query.exec(sql);
    query.next();
    int pet_num = query.value("pet_num").toInt();

    if(pet_num == 0)
    {
        QList<QPair<QString, QString>> initialSpirits ={
                                                         {"炎火猴","Attack" },
                                                         {"仙人球","Attack"},
                                                         {"贝尔","Defend"},
                                                         {"火炎贝","Defend"},
                                                         {"布布种子","Hp"},
                                                         {"伊修","Hp"},
                                                         {"比比鼠","Speed"},
                                                         {"悠悠","Speed"}};

        // 随机分配一只小精灵给用户
        srand((unsigned)time(NULL));
        for (int i = 0; i < 1; i++) {
            int index = rand() % initialSpirits.size();
            QString petName = initialSpirits[index].first; // 获取小精灵名称
            QString spiritType = initialSpirits[index].second; // 获取小精灵类型
            qDebug()<<userID<<petName<<spiritType<<Qt::endl;
            // 插入小精灵记录到数据库
            QString insertSpiritQuery = QString("INSERT INTO spirit (user_ID, spirit_name, type) VALUES (%1, '%2', '%3');")
                                            .arg(userID) // 这里的userID是你当前用户的ID
                                            .arg(petName)
                                            .arg(spiritType);
            query.exec(insertSpiritQuery);

            // 从可供随机分配的小精灵列表中移除已经分配的小精灵，避免重复分配
            initialSpirits.remove(index);
        }
        QString sql = QString("UPDATE user SET pet_num = 1 WHERE user_ID = %1").arg(userID);
        query.exec(sql);
        query.next();
    }
}
//平局或者失败获得经验和钱
void Widget::Exp(QJsonObject json) const
{
    int userID = json["user_ID"].toInt();
    int drug = json["drug"].toInt();
    QJsonObject spirit = json["spirit"].toObject();

    int spiritID = spirit["spirit_ID"].toInt();
    QString spiritName = spirit["spirit_name"].toString();
    QString evolution = spirit["evolution"].toString();
    int level = spirit["level"].toInt();
    int experience = spirit["experience"].toInt();
    int attack = spirit["attack"].toInt();
    int hp = spirit["hp"].toInt();
    int defense = spirit["defense"].toInt();
    double attackInterval = spirit["attackInterval"].toDouble(); // 注意：attackInterval 为 double

    // 确保数据库连接已经打开
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        qDebug() << "数据库未打开！";
        return;
    }

    QSqlQuery query(db);
    query.prepare("UPDATE spirit SET spirit_name = :spiritName, evolution = :evolution, level = :level, "
                  "experience = :experience, attack = :attack, hp = :hp, defense = :defense, attackInterval = :attackInterval "
                  "WHERE spirit_ID = :spiritID AND user_ID = :userID");

    query.bindValue(":spiritName", spiritName);
    query.bindValue(":evolution", evolution);
    query.bindValue(":level", level);
    query.bindValue(":experience", experience);
    query.bindValue(":attack", attack);
    query.bindValue(":hp", hp);
    query.bindValue(":defense", defense);
    query.bindValue(":attackInterval", attackInterval); // 使用双精度绑定
    query.bindValue(":spiritID", spiritID);
    query.bindValue(":userID", userID);

    // 调试输出
    qDebug() << "执行的 SQL 查询: " << query.executedQuery();
    qDebug() << "绑定的参数: " << query.boundValues();


    if (!query.exec()) {
        qDebug() << "更新精灵信息失败: " << query.lastError();
        // 适当处理错误
    } else {
        qDebug() << "精灵信息更新成功！";
    }
    QString sql = QString("UPDATE user SET total_games = total_games + 1, money = money + 200, drug = '%1' WHERE user_ID = '%2';").arg(drug).arg(userID);
    if (!query.exec(sql)) {
        qDebug() << "战斗场数、金钱数、药数更新失败" << query.lastError();
    }
    else{
        QString updateWinRatioQuery = QString("UPDATE user SET win_ratio = CAST(victory_count AS REAL) / total_games WHERE user_ID = '%1';").arg(userID);
        if (!query.exec(updateWinRatioQuery)) {
            qDebug() << "更新胜率失败：" << query.lastError();
        }
    }
}

void Widget::Lobby(QJsonObject json) const
{
    int userID = json["user_ID"].toInt();
    int client_socketID = json["socketID"].toInt();
    QSqlQuery query;
    QJsonArray spiritsArray;

    QString selectQuery = QString("SELECT * FROM spirit WHERE user_ID = %1").arg(userID);

    QString sql = QString("SELECT * FROM user WHERE user_ID = %1").arg(userID);
    query.exec(sql);
    query.next();
    int pet_num = query.value("pet_num").toInt();
    int money = query.value("money").toInt();
    int drug = query.value("drug").toInt();
    int medal1 = query.value("medal1").toInt();
    int medal2 = query.value("medal2").toInt();
    if (query.exec(selectQuery)) {
        while (query.next()) {
            QJsonObject spirit;
            spirit["spirit_ID"] = query.value("spirit_ID").toInt();
            spirit["user_ID"] = query.value("user_ID").toInt();
            spirit["evolution"] = query.value("evolution").toString();
            spirit["spirit_name"] = query.value("spirit_name").toString();
            spirit["type"] = query.value("type").toString();
            spirit["level"] = query.value("level").toInt();
            spirit["experience"] = query.value("experience").toInt();
            spirit["attack"] = query.value("attack").toInt();
            spirit["hp"] = query.value("hp").toInt();
            spirit["defense"] = query.value("defense").toInt();
            spirit["attackInterval"] = query.value("attackInterval").toDouble();

            spiritsArray.append(spirit);
        }
    } else {
        qDebug() << "查询精灵失败：" << query.lastError().text();
    }
    json = {
        {"kind", LOBBY},
        {"socketID", client_socketID},
        {"spirit", spiritsArray},
        {"pet_num", pet_num},
        {"money", money},
        {"drug", drug},
        {"medal1",medal1},
        {"medal2",medal2}
    };
    SendMessage(json);
}

void Widget::Login(QJsonObject json) const
{
    QSqlQuery query;
    QString username = json["username"].toString();
    QString password = json["password"].toString();
    QString sql_password;
    int userID;
    int pet_num;
    int client_socketID = json["socketID"].toInt();
    qDebug()<<"username:"<<username<<Qt::endl;
    QString sql = QString("SELECT password, user_ID, pet_num, online FROM user WHERE user_name = '%1';").arg(username);

    qDebug()<<"sql:"<<sql<<Qt::endl;
    query.exec(sql);
    if(query.next()){
        sql_password = query.value(0).toString();
        userID = query.value(1).toInt();
        pet_num = query.value(2).toInt();
        qDebug()<<"密码是"<<sql_password<<Qt::endl;
        //假如密码正确
        if(sql_password == password)
        {
            qDebug()<<"密码正确"<<Qt::endl;
            json  = {
                {"kind", LOGIN_SUCCEED},
                {"socketID", client_socketID},
                {"username", username},
                {"pet_num",pet_num},
                {"userID", userID}
            };
            int online = query.value(3).toInt();
            if(online == 1)
            {
                json  = {
                    {"kind", LOGIN_ONLINE},
                    {"socketID", client_socketID},
                };
            }
            qDebug()<<QString(QJsonDocument(json).toJson())<<Qt::endl;
            //更新数据库，设置用户登录状态为登录
            sql = QString("UPDATE user SET online = 1 WHERE user_name = '%1';").arg(username);
            query.exec(sql);

        }
        //密码错误
        else
        {
            json  = {
                {"kind", LOGIN_FAILED},
                {"socketID", client_socketID}
            };
        }
        qDebug()<<"服务器传回："<<json<<Qt::endl;
        SendMessage(json);

    }else{
        qDebug()<<"用户名不存在"<<Qt::endl;
        json = {
            {"kind",LOGIN_UNEXISTED},
            {"socketID", client_socketID}
        };
        SendMessage(json);
    }

}

void Widget::SendMessage(QJsonObject json) const
{
    int size = socketList.size();
    qDebug()<<"向"<<size<<"个客户发送信息"<<Qt::endl;
    //给所有的客户端发送信息
    for(int i = 0; i < size; i++)
    {
        QTcpSocket *temp = socketList[i];
        temp->write(QString(QJsonDocument(json).toJson()).toUtf8().data());
    }
}

void Widget::Register(QJsonObject json) const{
    QSqlQuery query;
    QString username = json["username"].toString();
    QString password = json["password"].toString();
    int client_socketID = json["socketID"].toInt();
    // 检查用户名是否已经存在
    QString checkQuery = QString("SELECT COUNT(*) FROM user WHERE user_name = '%1';").arg(username);
    query.exec(checkQuery);
    query.next();
    int count = query.value(0).toInt();
    if(count > 0) {
        // 用户名已存在，向客户端发送消息
        json = {
            {"kind", REGISTER_FAILED},
            {"socketID", client_socketID}
        };
        SendMessage(json);
    } else {
        // 用户名可用，进行注册
        qDebug()<<"插入用户信息"<<Qt::endl;
        QString insertQuery = QString("INSERT INTO user (user_name, password, online,pet_num) VALUES ('%1', '%2', 0, 3);")
                                  .arg(username)
                                  .arg(password)
                                  .arg(0).arg(3);
        if(query.exec(insertQuery)) {
            // 注册成功，向客户端发送成功消息
            json = {
                {"kind", REGISTER_SUCCEED},
                {"socketID", client_socketID}
            };
            SendMessage(json);

            //查找用户ID
            QString  sql = QString("SELECT user_ID FROM user WHERE user_name = '%1' ;").arg(username);
            query.exec(sql);
            query.first();
            int ID = query.value(0).toInt();
            qDebug()<<"ID:"<<ID<<Qt::endl;
            // 查询可供随机分配的小精灵
            QList<QPair<QString, QString>> initialSpirits ={
                                                             {"炎火猴","Attack" },
                                                             {"仙人球","Attack"},
                                                             {"贝尔","Defend"},
                                                             {"火炎贝","Defend"},
                                                             {"布布种子","Hp"},
                                                             {"伊修","Hp"},
                                                             {"比比鼠","Speed"},
                                                             {"悠悠","Speed"}};

            // 随机分配三只小精灵给用户
            srand((unsigned)time(NULL));
            for (int i = 0; i < 3; i++) {
                int index = rand() % initialSpirits.size();
                QString petName = initialSpirits[index].first; // 获取小精灵名称
                QString spiritType = initialSpirits[index].second; // 获取小精灵类型
                qDebug()<<ID<<petName<<spiritType<<Qt::endl;
                // 插入小精灵记录到数据库
                QString insertSpiritQuery = QString("INSERT INTO spirit (user_ID, spirit_name, type) VALUES (%1, '%2', '%3');")
                                                .arg(ID) // 这里的userID是你当前用户的ID
                                                .arg(petName)
                                                .arg(spiritType);
                query.exec(insertSpiritQuery);

                // 从可供随机分配的小精灵列表中移除已经分配的小精灵，避免重复分配
                initialSpirits.remove(index);
            }


        } else {
            // 注册失败，向客户端发送失败消息
            json = {
                {"kind", REGISTER_FAILED},
                {"socketID", client_socketID}
            };
            SendMessage(json);
        }
    }

}
void Widget::Log_out(QJsonObject json) const
{
    qDebug()<<"登出"<<Qt::endl;
    int user_ID = json["user_ID"].toInt();
    qDebug()<<user_ID<<Qt::endl;
    QSqlQuery query;
    QString sql;
    sql = QString("UPDATE user SET online = 0 WHERE user_ID = %1;").arg(user_ID);
    query.exec(sql);
}

void Widget::Bag(QJsonObject json) const
{
    int userID = json["user_ID"].toInt();
    int client_socketID = json["socketID"].toInt();
    QSqlQuery query;
    QJsonArray spiritsArray;

    QString selectQuery = QString("SELECT * FROM spirit WHERE user_ID = %1").arg(userID);

    QString sql = QString("SELECT pet_num FROM user WHERE user_ID = %1").arg(userID);
    query.exec(sql);
    query.next();
    int pet_num = query.value("pet_num").toInt();
    if (query.exec(selectQuery)) {
        while (query.next()) {
            QJsonObject spirit;
            spirit["spirit_ID"] = query.value("spirit_ID").toInt();
            spirit["user_ID"] = query.value("user_ID").toInt();
            spirit["evolution"] = query.value("evolution").toString();
            spirit["spirit_name"] = query.value("spirit_name").toString();
            spirit["type"] = query.value("type").toString();
            spirit["level"] = query.value("level").toInt();
            spirit["experience"] = query.value("experience").toInt();
            spirit["attack"] = query.value("attack").toInt();
            spirit["hp"] = query.value("hp").toInt();
            spirit["defense"] = query.value("defense").toInt();
            spirit["attackInterval"] = query.value("attackInterval").toDouble();

            spiritsArray.append(spirit);
        }
    } else {
        qDebug() << "查询精灵失败：" << query.lastError().text();
    }
    json = {
        {"kind", BAG},
        {"socketID", client_socketID},
        {"spirit", spiritsArray},
        {"pet_num", pet_num}
    };
    SendMessage(json);
}

Widget::~Widget()
{
    delete ui;
}
