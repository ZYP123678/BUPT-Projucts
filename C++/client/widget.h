#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMessageBox>//消息盒子
#include <QDebug>
#include<QTcpSocket>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QApplication>
#include "login.h"


void sqlite_Init();
QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);

    ~Widget();


private slots:
    void on_start_clicked();

private:
    Ui::Widget *ui;
    Login *login = NULL;
};
#endif // WIDGET_H
