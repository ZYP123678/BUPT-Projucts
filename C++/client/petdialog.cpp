#include "petdialog.h"
#include "ui_petdialog.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

PetDialog::PetDialog(QJsonArray spirits, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PetDialog),
    spirits(spirits)
{
    // Setup UI manually
    QVBoxLayout *layout = new QVBoxLayout(this);

    for (int i = 0; i < spirits.size(); ++i) {
        QJsonObject spirit = spirits[i].toObject();
        QString name = spirit["spirit_name"].toString();
        int level = spirit["level"].toInt();
        QString imagePath = ":/images/" + spirit["image"].toString();  // 假设图片路径在资源文件中

        QPushButton *button = new QPushButton;
        button->setIcon(QIcon(imagePath));
        button->setIconSize(QSize(50, 50));
        button->setText(QString("%1 (Level: %2)").arg(name).arg(level));
        button->setObjectName(QString::number(i));

        connect(button, &QPushButton::clicked, [this, i]() { onPetClicked(i); });

        layout->addWidget(button);
    }

    setLayout(layout);
}

PetDialog::~PetDialog()
{
    delete ui;
}

void PetDialog::onPetClicked(int index)
{
    emit petSelected(index);
    accept();
}
