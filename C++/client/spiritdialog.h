#ifndef SPIRITDIALOG_H
#define SPIRITDIALOG_H

#include <QDialog>
#include <QJsonArray>
#include<qjsonobject.h>
#include<qboxlayout.h>
#include <QScrollArea>
#include <QRandomGenerator>

namespace Ui {
class spiritdialog;
}

class spiritdialog : public QDialog
{
    Q_OBJECT

public:
    explicit spiritdialog(QJsonArray spirits, QWidget *parent = nullptr);
    ~spiritdialog();
signals:
    void petSelected(int index);

private slots:
    void onPetClicked(int index);



private:
    Ui::spiritdialog *ui;
    QJsonArray spirits;
    QScrollArea *scrollArea;
};

#endif // SPIRITDIALOG_H
