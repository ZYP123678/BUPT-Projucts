#include "widget.h"
#include "ui_widget.h"



Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)

{

    ui->setupUi(this);
    QPixmap *pix = new QPixmap(":/new/prefix1/zT44IFVH.png");
    QSize sz = ui->background->size();
    ui->background->setPixmap(pix->scaled(sz));

}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_start_clicked()
{
    login = new Login();
    login->show();
    this->close();
}

