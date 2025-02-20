#include "spiritdialog.h"
#include "qpushbutton.h"
#include "ui_spiritdialog.h"

spiritdialog::spiritdialog(QJsonArray spirits, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::spiritdialog)
{
    ui->setupUi(this);

    // 创建一个 QScrollArea
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);

    // 创建一个 QWidget 作为内容区域
    QWidget *contentWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(contentWidget);

    // 添加按钮到内容区域
    for (int i = 0; i < spirits.size(); ++i) {
        QJsonObject spirit = spirits[i].toObject();
        QString name = spirit["spirit_name"].toString();
        int level = spirit["level"].toInt();
        int Grade = 0;
        if (spirit["evolution"] == "Normal")
            Grade = 0;
        else if (spirit["evolution"] == "Evolution")
            Grade = 1;
        else if (spirit["evolution"] == "SuperEvolution")
            Grade = 2;
        QString imagePath;
        if (name == "炎火猴") {
            if (Grade == 0)
                imagePath = ":/attack/FireMonkey1.svg";
            else if (Grade == 1)
                imagePath = ":/attack/FireMonkey2.svg";
            else if (Grade == 2)
                imagePath = ":/attack/FireMonkey3.svg";
        }
        if (name == "仙人球") {
            if (Grade == 0)
                imagePath = ":/attack/Cactus1.svg";
            else if (Grade == 1)
                imagePath = ":/attack/Cactus2.svg";
            else if (Grade == 2)
                imagePath = ":/attack/Cactus3.svg";
        }
        if (name == "贝尔") {
            if (Grade == 0)
                imagePath = ":/defend/tortoise1.svg";
            else if (Grade == 1)
                imagePath = ":/defend/tortoise2.svg";
            else if (Grade == 2)
                imagePath = ":/defend/tortoise3.svg";
        }
        if (name == "火炎贝") {
            if (Grade == 0)
                imagePath = ":/defend/shell1.svg";
            else if (Grade == 1)
                imagePath = ":/defend/shell2.svg";
            else if (Grade == 2)
                imagePath = ":/defend/shell3.svg";
        }
        if (name == "布布种子") {
            if (Grade == 0)
                imagePath = ":/hp/seed1.svg";
            else if (Grade == 1)
                imagePath = ":/hp/seed2.svg";
            else if (Grade == 2)
                imagePath = ":/hp/seed3.svg";
        }
        if (name == "伊修") {
            if (Grade == 0)
                imagePath = ":/hp/undine1.svg";
            else if (Grade == 1)
                imagePath = ":/hp/undine2.svg";
            else if (Grade == 2)
                imagePath = ":/hp/undine3.svg";
        }
        if (name == "比比鼠") {
            if (Grade == 0)
                imagePath = ":/speed/mouse1.svg";
            else if (Grade == 1)
                imagePath = ":/speed/mouse2.svg";
            else if (Grade == 2)
                imagePath = ":/speed/mouse3.svg";
        }
        if (name == "悠悠") {
            if (Grade == 0)
                imagePath = ":/speed/bat1.svg";
            else if (Grade == 1)
                imagePath = ":/speed/bat2.svg";
            else if (Grade == 2)
                imagePath = ":/speed/bat3.svg";
        }

        QPushButton *button = new QPushButton;
        button->setIcon(QIcon(imagePath));
        button->setIconSize(QSize(250, 250));
        button->setText(QString("%1 (Level: %2)").arg(name).arg(level));
        button->setObjectName(QString::number(i));

        connect(button, &QPushButton::clicked, [this, i]() { onPetClicked(i); });

        layout->addWidget(button);
    }

    // 设置内容区域的布局
    contentWidget->setLayout(layout);

    // 将内容区域设置为 QScrollArea 的内容
    scrollArea->setWidget(contentWidget);

    // 设置主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(scrollArea);
    setLayout(mainLayout);
}

spiritdialog::~spiritdialog()
{
    delete ui;
}

void spiritdialog::onPetClicked(int index)
{
    emit petSelected(index);
    accept();
}

