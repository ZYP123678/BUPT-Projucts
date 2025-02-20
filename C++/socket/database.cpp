#include "database.h"
#include <QSqlQuery>
#include <QDebug>
#include <QDir>

Database::Database()
{
    openDatabase(); // 打开或创建数据库
}

bool Database::openDatabase() {
    QString dbPath = QDir::currentPath() + "/PrimitiveSpirit.db";

    // 检查数据库文件是否已经存在
    bool fileExists = QFile::exists(dbPath);

    // 添加数据库连接
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE"); // 使用 SQLite
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        // 数据库连接失败
        qDebug() << "无法打开数据库：" << db.lastError().text();
        return false;
    }

    qDebug() << "数据库连接成功";

    if (!fileExists) {
        // 如果数据库文件不存在，则创建必要的表
        if (!createTables()) {
            // 创建表失败
            qDebug() << "无法创建表";
            return false;
        }
    }

    return true;
}

bool Database::createTables() {
    QSqlQuery query;

    // 创建用户表
    QString createUserTableQuery = "CREATE TABLE IF NOT EXISTS user ("
                                   "user_ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                                   "user_name VARCHAR(255) NOT NULL,"
                                   "password VARCHAR(255) NOT NULL,"
                                   "online INT DEFAULT 0,"          //是否在线
                                   "pet_num INT DEFAULT 0,"         //精灵数
                                   "victory_count INT DEFAULT 0,"   // 胜利场数
                                   "total_games INT DEFAULT 0,"     // 总场数
                                   "win_ratio REAL DEFAULT 0.0,"    // 胜率
                                   "medal1 INT DEFAULT 0,"      // 勋章1
                                   "medal2 INT DEFAULT 0,"       // 勋章2
                                   "money INT DEFAULT 0,"            //金币数
                                   "drug INT DEFAULT 0"              //药数
                                   ")";
    if (!query.exec(createUserTableQuery)) {
        qDebug() << "创建用户表失败：" << query.lastError().text();
        return false;
    }

    qDebug() << "用户表创建成功";

    //创建精灵表
    QString createSpiritTableQuery = "CREATE TABLE IF NOT EXISTS spirit ("
                                     "spirit_ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                                     "user_ID INT NOT NULL,"        // 用户ID，用于关联用户
                                     "spirit_name VARCHAR(255) NOT NULL,"//精灵名字
                                     "type VARCHAR(50) NOT NULL,"   // 精灵种类
                                     "level INT DEFAULT 1,"         //精灵等级
                                     "experience INT DEFAULT 0,"    //精灵经验
                                     "attack INT DEFAULT 8,"        //精灵攻击，默认8
                                     "hp INT DEFAULT 100,"          // 血量，默认值为100
                                     "defense INT DEFAULT 5,"       //防御，默认值5
                                     "attackInterval DOUBLE DEFAULT 2.5," //攻击间隔2.5
                                     "evolution VARCHAR(50) DEFAULT Normal,"     //进化等级
                                     "FOREIGN KEY (user_ID) REFERENCES user(user_ID)"
                                     ")";
    if (!query.exec(createSpiritTableQuery)) {
        qDebug() << "创建精灵表失败：" << query.lastError().text();
        return false;
    }

    qDebug() << "精灵表创建成功";
    // 初始化初始精灵信息，并插入到精灵表中
    QList<QPair<QString, QString>> initialSpirits ={
                                     {"炎火猴","Attack" },
                                     {"仙人球","Attack"},
                                     {"贝尔","Defend"},
                                     {"火炎贝","Defend"},
                                     {"布布种子","Hp"},
                                     {"伊修","Hp"},
                                     {"比比鼠","Speed"},
                                     {"悠悠","Speed"}}; // 初始精灵名称列表
 for (const auto &spiritInfo : initialSpirits) {
        const QString &spiritName = spiritInfo.first;
        const QString &spiritType = spiritInfo.second;
        QString insertSpiritQuery = QString("INSERT INTO spirit (user_ID, spirit_name, type) VALUES (0, '%1', '%2');")
                                        .arg(spiritName)
                                        .arg(spiritType);

        if (!query.exec(insertSpiritQuery)) {
            qDebug() << "插入初始精灵失败：" << query.lastError().text();
            return false;
        }
    }

    return true;
}

void Database::closeDatabase() {
    QSqlDatabase::database().close();
}

QSqlDatabase Database::getDatabase() {
    return QSqlDatabase::database();
}
