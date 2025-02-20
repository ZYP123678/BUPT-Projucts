#include "spiritinfodialog.h"
#include "qjsonobject.h"
#include "ui_spiritinfodialog.h"

SpiritInfoDialog::SpiritInfoDialog(QJsonArray spirits, int petnum, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SpiritInfoDialog)
{
    pet_num = petnum;
    spiritsArray = spirits;
    ui->setupUi(this);
    qDebug()<<pet_num<<spiritsArray<<Qt::endl;
    parseSpiritInfo();
    updateUi();
}

SpiritInfoDialog::~SpiritInfoDialog()
{
    delete ui;
}

void SpiritInfoDialog::parseSpiritInfo() {
    spiritInfos.clear(); // 清空旧数据
    for (const QJsonValue &value : spiritsArray) {
        QJsonObject spirit = value.toObject();
        QString spiritInfo = QString("ID: %1, Evolution: %2, Name: %3, Type: %4, Level: %5, Exp: %6, ATK: %7, HP: %8, DEF: %9, Interval: %10")
                                 .arg(spirit["spirit_ID"].toInt())
                                 .arg(spirit["evolution"].toString())
                                 .arg(spirit["spirit_name"].toString())
                                 .arg(spirit["type"].toString())
                                 .arg(spirit["level"].toInt())
                                 .arg(spirit["experience"].toInt())
                                 .arg(spirit["attack"].toInt())
                                 .arg(spirit["hp"].toInt())
                                 .arg(spirit["defense"].toInt())
                                 .arg(spirit["attackInterval"].toDouble());
        spiritInfos.append(spiritInfo);
    }
}

void SpiritInfoDialog::updateUi() {
    if (spiritInfos.isEmpty()) return;

    QString spiritInfo = spiritInfos[pet_index];
    QStringList parts = spiritInfo.split(", ");
    QString name;
    for (const QString &part : parts) {
        QStringList keyValue = part.split(": ");
        if (keyValue.length() == 2) {
            QString key = keyValue[0].trimmed();
            QString value = keyValue[1].trimmed();

            if (key == "Interval") {
                ui->attack_interval->setText(value);
            } else if (key == "Name") {
                ui->name->setText(value);
                name = value;

            } else if (key == "Type") {
                if (value == "Attack")
                    ui->type->setText("攻击型");
                else if (value == "Defend")
                    ui->type->setText("防御型");
                else if (value == "Hp")
                    ui->type->setText("肉盾型");
                else if (value == "Speed")
                    ui->type->setText("敏捷型");
            } else if (key == "Level") {
                ui->level->setText(value);
                int level = value.toInt();
                displaySpiritImage(name, level);
            } else if (key == "Exp") {
                ui->exp->setText(value);
            } else if (key == "ATK") {
                ui->attack->setText(value);
            } else if (key == "HP") {
                ui->hp->setText(value);
            } else if (key == "DEF") {
                ui->defense->setText(value);
            } else if (key == "Evolution") {
                ui->evolution->setText(value);
            }
        }
    }
}

void SpiritInfoDialog::displaySpiritImage(const QString &name, int level) {
    QString Image;
    Image = getImagePath(name, level);

    QPixmap originalPix(Image);
    if (!originalPix.isNull()) {
        QSize newSize(originalPix.size() * 4); // 将图像大小增加四倍
        QPixmap scaledPix = originalPix.scaled(newSize, Qt::KeepAspectRatio, Qt::FastTransformation); // 快速放大

        QSize sz = ui->pic->size();
        ui->pic->setPixmap(scaledPix.scaled(sz, Qt::KeepAspectRatio, Qt::SmoothTransformation)); // 平滑缩小
    }
}



QString SpiritInfoDialog::getImagePath(const QString &name, int level) {
    QString imagePath;

    if (name == "炎火猴") {
        if (level < 5) imagePath = ":/attack/FireMonkey1.svg";
        else if (level < 10) imagePath = ":/attack/FireMonkey2.svg";
        else imagePath = ":/attack/FireMonkey3.svg";

        ui->skill_name_2->setText("火球爆炸");
        ui->skill_effect_2->setText("附加0.5倍攻击伤害的爆炸伤害");
    } else if (name == "仙人球") {
        if (level < 5) imagePath = ":/attack/Cactus1.svg";
        else if (level < 10) imagePath = ":/attack/Cactus2.svg";
        else imagePath = ":/attack/Cactus3.svg";

        ui->skill_name_2->setText("尖刺");
        ui->skill_effect_2->setText("穿透攻击，每次攻击命中，若敌方防御力大于3则防御力减2");
    } else if (name == "贝尔") {
        if (level < 5) imagePath = ":/defend/tortoise1.svg";
        else if (level < 10) imagePath = ":/defend/tortoise2.svg";
        else imagePath = ":/defend/tortoise3.svg";

        ui->skill_name_2->setText("龟壳增生");
        ui->skill_effect_2->setText("每回合防御加一");
    } else if (name == "火炎贝") {
        if (level < 5) imagePath = ":/defend/shell1.svg";
        else if (level < 10) imagePath = ":/defend/shell2.svg";  
        else imagePath = ":/defend/shell3.svg";

        ui->skill_name_2->setText("火焰防御");
        ui->skill_effect_2->setText("若对方攻击力大于防御力加五，则每回合减1攻击力");
    } else if (name == "布布种子") {
        if (level < 5) imagePath = ":/hp/seed1.svg";
        else if (level < 10) imagePath = ":/hp/seed2.svg";
        else imagePath = ":/hp/seed3.svg";

        ui->skill_name_2->setText("光合作用");
        ui->skill_effect_2->setText("每回合给自己恢复20生命");
    } else if (name == "伊修") {
        if (level < 5) imagePath = ":/hp/undine1.svg";
        else if (level < 10) imagePath = ":/hp/undine2.svg";
        else imagePath = ":/hp/undine3.svg";

        ui->skill_name_2->setText("沸腾");
        ui->skill_effect_2->setText("每次攻击附带已扣除血量的一部分伤害");
    } else if (name == "悠悠") {
        if (level < 5) imagePath = ":/speed/bat1.svg";
        else if (level < 10) imagePath = ":/speed/bat2.svg";
        else imagePath = ":/speed/bat3.svg";

        ui->skill_name_2->setText("超声波探测");
        ui->skill_effect_2->setText("每次攻击减少自身攻击间隔0.05秒");
    } else if (name == "比比鼠") {
        if (level < 5) imagePath = ":/speed/mouse1.svg";
        else if (level < 10) imagePath = ":/speed/mouse2.svg";
        else imagePath = ":/speed/mouse3.svg";

        ui->skill_name_2->setText("雷电麻痹");
        ui->skill_effect_2->setText("每次攻击减慢敌方攻击间隔0.1秒");

    }

    return imagePath;
}

void SpiritInfoDialog::on_last_2_clicked()
{
    pet_index = (pet_index - 1  + pet_num)%pet_num;
    updateUi();
}


void SpiritInfoDialog::on_all_2_clicked()
{
    spiritdialog dialog(spiritsArray, this);
    connect(&dialog, &spiritdialog::petSelected, this, &SpiritInfoDialog::handlePetSelected);
    dialog.exec();
}

void SpiritInfoDialog::handlePetSelected(int index)
{
    pet_index = index;
    updateUi();
}


void SpiritInfoDialog::on_next_2_clicked()
{
    pet_index = (pet_index + 1 + pet_num)%pet_num;
    updateUi();
}

