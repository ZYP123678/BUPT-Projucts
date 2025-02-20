#ifndef SPIRITINFODIALOG_H
#define SPIRITINFODIALOG_H

#include <QDialog>
#include <QJsonArray>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <spiritdialog.h>

namespace Ui {
class SpiritInfoDialog;
}

class SpiritInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SpiritInfoDialog(QJsonArray spirits, int petnum, QWidget *parent = nullptr);
    QString getImagePath(const QString &name, int level);
    void updateUi();
    void parseSpiritInfo();
    void displaySpiritImage(const QString &name, int grade);
    ~SpiritInfoDialog();

private slots:
    void on_last_2_clicked();

    void on_all_2_clicked();

    void on_next_2_clicked();

    void handlePetSelected(int index);//处理选中的精灵


private:
    Ui::SpiritInfoDialog *ui;
    QJsonArray spiritsArray;
    int pet_index = 0;
    QStringList spiritInfos;
    int pet_num;

};

#endif // SPIRITINFODIALOG_H
