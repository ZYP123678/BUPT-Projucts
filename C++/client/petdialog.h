#ifndef PETDIALOG_H
#define PETDIALOG_H

#include <QDialog>
#include <QJsonArray>
#include<qjsonobject.h>

namespace Ui {
class PetDialog;
}

class PetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PetDialog(QJsonArray spirits, QWidget *parent = nullptr);
    ~PetDialog();

signals:
    void petSelected(int index);

private slots:
    void onPetClicked(int index);

private:
    Ui::PetDialog *ui;
    QJsonArray spirits;
};

#endif // PETDIALOG_H
