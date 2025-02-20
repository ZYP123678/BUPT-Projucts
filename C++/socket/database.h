#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QDir>
class Database {
public:
    Database();
    static bool openDatabase();
    static bool createTables();
    static void closeDatabase();
    static QSqlDatabase getDatabase();
};

#endif // DATABASE_H
