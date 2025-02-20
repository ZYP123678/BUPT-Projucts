#include "widget.h"

#include <QApplication>
Manager manage;
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.show();
    return a.exec();
}
