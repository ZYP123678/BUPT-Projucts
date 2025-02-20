#include "widget.h"
#include<database.h>
#include <QApplication>
Database da;
int main(int argc, char *argv[])
{

    QApplication a(argc, argv);
    if (!Database::openDatabase()) {
        return 1; // 连接数据库失败，退出应用
    }
    Widget w;
    w.show();
    return a.exec();
}
