#include "lobby.h"
#include "qgraphicsitem.h"
#include "ui_lobby.h"

Lobby::Lobby(QTcpSocket *tcpsocket,QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Lobby)
    , socket(tcpsocket)
{
    ui->setupUi(this);
    socket = tcpsocket;

    //设置图片
    QPixmap *pix = new QPixmap(":/new/prefix1/main_lobby.jpg");
    QSize sz = ui->main_lobby_back->size();
    ui->main_lobby_back->setPixmap(pix->scaled(sz));

    pix = new QPixmap(":/new/prefix1/fight.jpg");
    sz = ui->background_fight->size();
    ui->background_fight->setPixmap(pix->scaled(sz));

    pix = new QPixmap(":/new/prefix1/money.png");
    sz = ui->money_image_1->size();
    ui->money_image_1->setPixmap(pix->scaled(sz));

    pix = new QPixmap(":/new/prefix1/blood.png");
    sz = ui->drug_iamge_1->size();
    ui->drug_iamge_1->setPixmap(pix->scaled(sz));

    pix = new QPixmap(":/new/prefix1/money.png");
    sz = ui->money_image_2->size();
    ui->money_image_2->setPixmap(pix->scaled(sz));

    pix = new QPixmap(":/new/prefix1/blood.png");
    sz = ui->drug_image_2->size();
    ui->drug_image_2->setPixmap(pix->scaled(sz));
    sz = ui->drug_image->size();
    ui->drug_image->setPixmap(pix->scaled(sz));
    sz = ui->drug_image_3->size();
    ui->drug_image_3->setPixmap(pix->scaled(sz));

    // 设置 user_list 的模型
    QStandardItemModel *model = new QStandardItemModel(this);
    model->setColumnCount(2);
    model->setHeaderData(0, Qt::Horizontal, "Name");
    model->setHeaderData(1, Qt::Horizontal, "Level");
    ui->user_list->setModel(model);
    // 设置列宽策略
    ui->user_list->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // 设置选择行为为整行
    ui->user_list->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->duelView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->expView->setSelectionBehavior(QAbstractItemView::SelectRows);
    // 设置选择模式为单选
    ui->user_list->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->duelView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->expView->setSelectionMode(QAbstractItemView::SingleSelection);
    // 添加刷新按钮
    QPushButton *refreshButton = new QPushButton("刷新", this);

    // 设置按钮的大小
    refreshButton->setFixedSize(80, 40); // 设置按钮为宽80像素，高30像素

    // 创建水平布局
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addStretch(); // 在按钮之前添加一个伸缩空间，使按钮位于右侧
    layout->setContentsMargins(0, 0, 18, 0); // 设置布局的右边距为18像素
    // 将按钮添加到布局
    layout->addWidget(refreshButton);

    // 将布局设置给 QTabWidget
    ui->tab_host->setLayout(layout);
    // 连接刷新按钮
    connect(refreshButton, &QPushButton::clicked, this, &Lobby::refreshPets);
    QString soundFilePath;
    // 根据页面索引设置不同的音频文件路径
    soundFilePath = ":/music/kannong.wav"; // 确保文件路径正确

    if (!soundFilePath.isEmpty()) {
        soundEffect = new QSoundEffect(this);
        soundEffect->setSource(QUrl::fromLocalFile(soundFilePath));
        soundEffect->setLoopCount(QSoundEffect::Infinite);//设置循环播放
        soundEffect->setVolume(0.5); // 设置音量
        soundEffect->play(); // 播放音频
    }

    //socket->connectToHost("127.0.0.1", 13520);
    //收到服务器回复
    connect(socket, SIGNAL(readyRead()), this, SLOT(receiveFromServer()));

    QJsonObject json  = {
        {"kind", LOBBY},
        {"socketID", manage.socketID},
        {"username", manage.username},
        {"user_ID", manage.userID}
    };
    socket->write(QString(QJsonDocument(json).toJson()).toUtf8().data());//进入就获取用户精灵数据
    ui->money_2->setText(QString::number(manage.money));
    ui->drug->setText(QString::number(manage.drug));

    connect(socket, &QAbstractSocket::disconnected, this, &Lobby::handleServerDisconnected);
}

void Lobby::handleServerDisconnected() {
    // 弹出一个消息框提示用户
    QMessageBox::critical(this, "连接断开", "与服务器的连接已断开，请检查网络或稍后再试");
    qApp->exit();
}


void Lobby::refreshPets()  {
    QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->user_list->model());
    if (!model) {
        qWarning() << "Model not set for user_list.";
        return; // 如果没有设置模型，直接返回
    }

    QModelIndexList selected = ui->user_list->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        qWarning() << "No pet selected in user_list.";
        QMessageBox::critical(this, "错误", "请选择你的出战精灵");
        return;
    }

    int selectedLevel = model->item(selected.first().row(), 1)->text().toInt();

    QTableView *expView = ui->tab_host->findChild<QTableView*>("expView");
    QTableView *duelView = ui->tab_host->findChild<QTableView*>("duelView");

    if (!expView || !duelView) {
        qWarning() << "Could not find expView or duelView.";
        return;
    }

    // 清空并设置模型
    QStandardItemModel *expModel = new QStandardItemModel(this);
    QStandardItemModel *duelModel = new QStandardItemModel(this);
    expModel->setColumnCount(2);
    duelModel->setColumnCount(2);
    expModel->setHeaderData(0, Qt::Horizontal, "Name");
    expModel->setHeaderData(1, Qt::Horizontal, "Level");
    duelModel->setHeaderData(0, Qt::Horizontal, "Name");
    duelModel->setHeaderData(1, Qt::Horizontal, "Level");


    expView->setModel(expModel);
    duelView->setModel(duelModel);

    // 生成随机精灵并添加到模型中
    for (int i = 0; i < 12; ++i) {
        int level;

        do {
            int randomOffset = QRandomGenerator::global()->bounded(7) - 3;
            level = selectedLevel + randomOffset; // 生成echo到3之间的随机数，确保等级至少为1
        } while (level <= 0 || level > 15); // 如果等级小于等于0或大于15，则重新生成
        pet *randomPet = pet::createRandomPet(level);

        QStandardItem *nameItemExp = new QStandardItem(randomPet->getName());
        QStandardItem *levelItemExp = new QStandardItem(QString::number(randomPet->getLevel()));

        QStandardItem *nameItemDuel = new QStandardItem(randomPet->getName());
        QStandardItem *levelItemDuel = new QStandardItem(QString::number(randomPet->getLevel()));

        // 设置项不可编辑
        nameItemExp->setFlags(nameItemExp->flags() & ~Qt::ItemIsEditable);
        levelItemExp->setFlags(levelItemExp->flags() & ~Qt::ItemIsEditable);
        nameItemDuel->setFlags(nameItemDuel->flags() & ~Qt::ItemIsEditable);
        levelItemDuel->setFlags(levelItemDuel->flags() & ~Qt::ItemIsEditable);

        expModel->setItem(i, 0, nameItemExp);
        expModel->setItem(i, 1, levelItemExp);

        duelModel->setItem(i, 0, nameItemDuel);
        duelModel->setItem(i, 1, levelItemDuel);
    }
}

Lobby::~Lobby()
{
    delete ui;
}


void Lobby::reconnect() const
{
    bool flag = connect(socket, SIGNAL(readyRead()), this, SLOT(receiveFromServer()));

    if(flag == true)
    {
        qDebug()<<"Lobby与服务器连接"<<Qt::endl;
    }
    else
    {
        qDebug()<<"Lobby未能与服务器连接"<<Qt::endl;
    }

}


void Lobby::on_all_2_clicked()
{
    spiritdialog dialog(spiritsArray, this);
    connect(&dialog, &spiritdialog::petSelected, this, &Lobby::handlePetSelected);
    dialog.exec();
}

void Lobby::handlePetSelected(int index)
{
    pet_index = index;
    updateUi();
}

void Lobby::on_tabWidget_tabBarClicked(int index)
{
    if(index == 1){
        QJsonObject json  = {
            {"kind", BAG},
            {"socketID", manage.socketID},
            {"username", manage.username},
            {"user_ID", manage.userID}
        };
        socket->write(QString(QJsonDocument(json).toJson()).toUtf8().data());
    }
}


void Lobby::receiveFromServer()
{
    //从通信套接字中取出内容
    QByteArray array = socket->readAll();
    QJsonDocument jsonDocument = QJsonDocument::fromJson(array);
    QJsonObject json = jsonDocument.object();
    if(json["socketID"].toInt() != manage.socketID)
    {
        return;
    }
    qDebug()<<"lobby收到服务器："<<json<<Qt::endl;
    if (json["kind"] == BAG) {
        if (json.contains("spirit") && json["spirit"].isArray()) {
            manage.pet_num = json["pet_num"].toInt();
            spiritsArray = json["spirit"].toArray();
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
                spiritInfos.append(spiritInfo); // 将精灵信息添加到容器中
            }
            // 在循环外部输出所有精灵信息
            for(const QString &spiritInfo : spiritInfos) {
                qDebug() << spiritInfo << Qt::endl;
            }
            updateUi();
        }
    }
    else if(json["kind"] == LOBBY){
        if (json.contains("spirit") && json["spirit"].isArray()) {
            manage.pet_num = json["pet_num"].toInt();
            manage.money = json["money"].toInt();
            manage.drug = json["drug"].toInt();
            spiritsArray = json["spirit"].toArray();
            setupUserListView(spiritsArray);
            ui->money_2->setText(QString::number(manage.money));
            ui->drug->setText(QString::number(manage.drug));
            ui->money_2->setText(QString::number(manage.money));
            ui->drug->setText(QString::number(manage.drug));

            int medal1 = json["medal1"].toInt();
            int medal2 = json["medal2"].toInt();
            QPixmap *pix1 = NULL;
            QPixmap *pix2 = NULL;
            if(medal1 == 0){
                ui->medal_1->setText("元气");
            }
            else if(medal1 == 1){
                pix1 = new QPixmap(":/medal/1.png");
            }
            else if(medal1 == 2){
                pix1 = new QPixmap(":/medal/2.png");
            }
            else if(medal1 == 3){
                pix1 = new QPixmap(":/medal/3.png");
            }

            if(medal1 != 0){
                QSize sz = ui->medal_1->size();
                ui->medal_1->setPixmap(pix1->scaled(sz));
            }

            if(medal2 == 0){
                ui->medal_2->setText("精灵");
            }
            else if(medal2 == 1){
                pix2 = new QPixmap(":/medal/4.png");
            }
            else if(medal2 == 2){
                pix2 = new QPixmap(":/medal/5.png");
            }
            else if(medal2 == 3){
                pix2 = new QPixmap(":/medal/6.png");
            }

            if(medal2 != 0){
                QSize sz = ui->medal_2->size();
                ui->medal_2->setPixmap(pix2->scaled(sz));
            }
        }
    }
    else if(json["kind"] == USERLIST){
        Userlist(json);
    }
}

void Lobby::updateUi()
{
    QString spiritInfo = spiritInfos[pet_index];
    QStringList parts = spiritInfo.split(", ");
    int Grade = 0;
    for(const QString &part : parts) {
        QStringList keyValue = part.split(": ");
        if (keyValue.length() == 2) {
            QString value = keyValue[1].trimmed();  // 去除值两端的空格
            if (keyValue[0].trimmed() == "Interval") {
                ui->attack_interval_2->setText(value);
            } else if (keyValue[0].trimmed() == "Name") {
                ui->name_2->setText(value);
                QPixmap pix;
                if(value == "炎火猴") {
                    if(Grade == 0)
                        pix.load(":/attack/FireMonkey1.svg");
                    else if(Grade == 1)
                        pix.load(":/attack/FireMonkey2.svg");
                    else if(Grade == 2)
                        pix.load(":/attack/FireMonkey3.svg");

                    ui->skill_name_2->setText("火球爆炸");
                    ui->skill_effect_2->setText("附加0.5倍攻击伤害的爆炸伤害");
                }
                if(value == "仙人球") {
                    if(Grade == 0)
                        pix.load(":/attack/Cactus1.svg");
                    else if(Grade == 1)
                        pix.load(":/attack/Cactus2.svg");
                    else if(Grade == 2)
                        pix.load(":/attack/Cactus3.svg");

                    ui->skill_name_2->setText("尖刺");
                    ui->skill_effect_2->setText("穿透攻击，每次攻击命中，若敌方防御力大于3则防御力减2");
                }
                if(value == "贝尔") {
                    if(Grade == 0)
                        pix.load(":/defend/tortoise1.svg");
                    else if(Grade == 1)
                        pix.load(":/defend/tortoise2.svg");
                    else if(Grade == 2)
                        pix.load(":/defend/tortoise3.svg");

                    ui->skill_name_2->setText("龟壳增生");
                    ui->skill_effect_2->setText("每回合防御加一");
                }
                if(value == "火炎贝") {
                    if(Grade == 0)
                        pix.load(":/defend/shell1.svg");
                    else if(Grade == 1)
                        pix.load(":/defend/shell2.svg");
                    else if(Grade == 2)
                        pix.load(":/defend/shell3.svg");

                    ui->skill_name_2->setText("火焰防御");
                    ui->skill_effect_2->setText("若对方攻击力大于防御力加五，则每回合减1攻击力");
                }
                if(value == "布布种子") {
                    if(Grade == 0)
                        pix.load(":/hp/seed1.svg");
                    else if(Grade == 1)
                        pix.load(":/hp/seed2.svg");
                    else if(Grade == 2)
                        pix.load(":/hp/seed3.svg");

                    ui->skill_name_2->setText("光合作用");
                    ui->skill_effect_2->setText("每回合给自己恢复20生命");
                }
                if(value == "伊修") {
                    if(Grade == 0)
                        pix.load(":/hp/undine1.svg");
                    else if(Grade == 1)
                        pix.load(":/hp/undine2.svg");
                    else if(Grade == 2)
                        pix.load(":/hp/undine3.svg");

                    ui->skill_name_2->setText("沸腾");
                    ui->skill_effect_2->setText("每次攻击附带已扣除血量的一部分伤害");
                }
                if(value == "比比鼠") {
                    if(Grade == 0)
                        pix.load(":/speed/mouse1.svg");
                    else if(Grade == 1)
                        pix.load(":/speed/mouse2.svg");
                    else if(Grade == 2)
                        pix.load(":/speed/mouse3.svg");

                    ui->skill_name_2->setText("雷电麻痹");
                    ui->skill_effect_2->setText("每次攻击减慢敌方攻击间隔0.1秒");
                }
                if(value == "悠悠") {
                    if(Grade == 0)
                        pix.load(":/speed/bat1.svg");
                    else if(Grade == 1)
                        pix.load(":/speed/bat2.svg");
                    else if(Grade == 2)
                        pix.load(":/speed/bat3.svg");

                    ui->skill_name_2->setText("超声波探测");
                    ui->skill_effect_2->setText("每次攻击减少自身攻击间隔0.05秒");
                }

                if(!pix.isNull()) {
                    QGraphicsScene *scene = new QGraphicsScene();
                    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pix);
                    scene->addItem(item);

                    // 将场景设置到 QGraphicsView 中
                    ui->pic_2->setScene(scene);

                    // 调整视图以适应场景大小
                    ui->pic_2->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
                }
            } else if (keyValue[0].trimmed() == "Type") {
                if (value == "Attack")
                    ui->type_2->setText("攻击型");
                else if (value == "Defend")
                    ui->type_2->setText("防御型");
                else if (value == "Hp")
                    ui->type_2->setText("肉盾型");
                else if (value == "Speed")
                    ui->type_2->setText("敏捷型");
            } else if (keyValue[0].trimmed() == "Level") {
                ui->level_2->setText(value);
            } else if (keyValue[0].trimmed() == "Exp") {
                ui->exp_2->setText(value);
            } else if (keyValue[0].trimmed() == "ATK") {
                ui->attack_2->setText(value);
            } else if (keyValue[0].trimmed() == "HP") {
                ui->hp_2->setText(value);
            } else if (keyValue[0].trimmed() == "DEF") {
                ui->defence_2->setText(value);
            } else if (keyValue[0].trimmed() == "Evolution") {
                ui->evolution_2->setText(value);
                if(value == "Normal")
                    Grade = 0;
                else if(value == "Evolution")
                    Grade = 1;
                else if(value == "SuperEvolution")
                    Grade = 2;
            }
        }
    }
}//背包显示精灵信息

void Lobby::on_log_out_2_clicked()
{
        QJsonObject json  = {
            {"kind", LOGOUT},
            {"socketID", manage.socketID},
            {"username", manage.username},
            {"user_ID", manage.userID}
        };
        qDebug()<<json<<Qt::endl;

        //向服务器发送信息
        socket->write(QString(QJsonDocument(json).toJson()).toUtf8().data());
        emit loggedOut();
        socket->disconnect();
        this->close();
        soundEffect->stop();
}//登出


void Lobby::on_last_2_clicked()
{
    pet_index = (pet_index - 1 + manage.pet_num)%manage.pet_num;
    qDebug()<<pet_index<<Qt::endl;
    updateUi();
}//上一只精灵信息


void Lobby::on_next_2_clicked()
{
    pet_index = (pet_index + 1 + manage.pet_num)%manage.pet_num;
    updateUi();
}//下一只精灵

void Lobby::setupUserListView(const QJsonArray &spirits) const
{
    QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->user_list->model());
    if (!model) {
        qWarning() << "Model not set for user_list.";
        return; // 如果没有设置模型，直接返回
    }

    model->removeRows(0, model->rowCount()); // 清除之前的数据
    model->setColumnCount(2); // 确保模型有两列

    qDebug() << spirits << Qt::endl;

    // 填充数据
    for (int i = 0; i < spirits.size(); ++i) {
        QJsonObject spirit = spirits[i].toObject();
        QString name = spirit["spirit_name"].toString();
        int level = spirit["level"].toInt();

        QStandardItem *nameItem = new QStandardItem(name);
        QStandardItem *levelItem = new QStandardItem(QString::number(level));
        // 设置项不可编辑
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        levelItem->setFlags(levelItem->flags() & ~Qt::ItemIsEditable);

        model->setItem(i, 0, nameItem);
        model->setItem(i, 1, levelItem);
    }
}
//显示用户精灵
pet::grade parseEvolution(const QString &evolutionStr) {
    if (evolutionStr == "Normal") return pet::Normal;
    else if (evolutionStr == "Evolution") return pet::Evolution;
    else if (evolutionStr == "SuperEvolution") return pet::SuperEvolution;
    else return pet::Normal; // 默认值
}

void Lobby::on_fight_button_2_clicked()
{
    ui->now_drug->setText(QString::number(manage.drug));
    // 获取用户选择的己方精灵
    QModelIndexList selectedIndexes = ui->user_list->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) {
        qDebug() << "未选择己方精灵";
        return;
    }
    int userSelectedRow = selectedIndexes.first().row();
    QStandardItemModel *userModel = qobject_cast<QStandardItemModel*>(ui->user_list->model());
    QString userName = userModel->item(userSelectedRow, 0)->text();
    int userLevel = userModel->item(userSelectedRow, 1)->text().toInt();
    QJsonObject obj = spiritsArray[userSelectedRow].toObject();//根据行号直接获取相应精灵信息
    QString name_1 = obj["spirit_name"].toString();
    int level_1 = obj["level"].toInt();
    int exp_1 = obj["experience"].toInt();
    pet::grade evolution_1 = parseEvolution(obj["evolution"].toString());
    int attack_1 = obj["attack"].toInt();
    int defense_1 = obj["defense"].toInt();
    int hp_1 = obj["hp"].toInt();
    double interval_1 = obj["attackInterval"].toDouble();
    int ID = obj["spirit_ID"].toInt();
    if (name_1 == "炎火猴") {
        s1 = new FireMonkey(level_1, evolution_1, exp_1, attack_1, defense_1, hp_1, interval_1);
    } else if (name_1 == "仙人球") {
        s1 = new Cactus(level_1, evolution_1, exp_1, attack_1, defense_1, hp_1, interval_1);
    } else if (name_1 == "贝尔") {
        s1 = new tortoise(level_1, evolution_1, exp_1, attack_1, defense_1, hp_1, interval_1);
    } else if (name_1 == "火炎贝") {
        s1 = new shell(level_1, evolution_1, exp_1, attack_1, defense_1, hp_1, interval_1);
    } else if (name_1 == "布布种子") {
        s1 = new seed(level_1, evolution_1, exp_1, attack_1, defense_1, hp_1, interval_1);
    } else if (name_1 == "伊修") {
        s1 = new undine(level_1, evolution_1, exp_1, attack_1, defense_1, hp_1, interval_1);
    } else if (name_1 == "悠悠") {
        s1 = new bat(level_1, evolution_1, exp_1, attack_1, defense_1, hp_1, interval_1);
    } else if (name_1 == "比比鼠") {
        s1 = new mouse(level_1, evolution_1, exp_1, attack_1, defense_1, hp_1, interval_1);
    } else {
        s1 = nullptr;
    }

    // 获取敌方精灵
    QModelIndexList expSelectedIndexes = ui->expView->selectionModel()->selectedIndexes();
    QModelIndexList duelSelectedIndexes = ui->duelView->selectionModel()->selectedIndexes();

    QString enemyName;
    int enemyLevel;

    if (!expSelectedIndexes.isEmpty()) {
        int enemySelectedRow = expSelectedIndexes.first().row();
        QStandardItemModel *expModel = qobject_cast<QStandardItemModel*>(ui->expView->model());
        enemyName = expModel->item(enemySelectedRow, 0)->text();
        enemyLevel = expModel->item(enemySelectedRow, 1)->text().toInt();
    } else if (!duelSelectedIndexes.isEmpty()) {
        int enemySelectedRow = duelSelectedIndexes.first().row();
        QStandardItemModel *duelModel = qobject_cast<QStandardItemModel*>(ui->duelView->model());
        enemyName = duelModel->item(enemySelectedRow, 0)->text();
        enemyLevel = duelModel->item(enemySelectedRow, 1)->text().toInt();
    } else {
        qDebug() << "未选择敌方精灵";
        return;
    }

    // 将精灵信息显示在战斗页面的 QLabel 上
    if(userName == "炎火猴")
    {
        if(userLevel < 5)
        {
            QPixmap *pix = new QPixmap(":/attack/FireMonkey1.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        else if(5 <= userLevel && userLevel <10)
        {
            QPixmap *pix = new QPixmap(":/attack/FireMonkey2.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        else if(userLevel >= 10 && userLevel <= 15)
        {
            QPixmap *pix = new QPixmap(":/attack/FireMonkey3.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        ui->s1_name->setText("炎火猴");
        ui->s1_hp->setText(QString("%1/%2").arg(FireMonkey(userLevel).getfight_HP()).arg(FireMonkey(userLevel).getHP()));
    }
    else if(userName == "仙人球")
    {
        if(userLevel < 5)
        {
            QPixmap *pix = new QPixmap(":/attack/Cactus1.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        else if(5 <= userLevel && userLevel <10)
        {
            QPixmap *pix = new QPixmap(":/attack/Cactus2.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        else if(userLevel >= 10 && userLevel <= 15)
        {
            QPixmap *pix = new QPixmap(":/attack/Cactus3.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        ui->s1_name->setText("仙人球");
        ui->s1_hp->setText(QString("%1/%2").arg(Cactus(userLevel).getfight_HP()).arg(Cactus(userLevel).getHP()));
    }
    else if(userName == "贝尔")
    {
        if(userLevel < 5)
        {
            QPixmap *pix = new QPixmap(":/defend/tortoise1.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        else if(5 <= userLevel && userLevel <10)
        {
            QPixmap *pix = new QPixmap(":/defend/tortoise2.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        else if(userLevel >= 10 && userLevel <= 15)
        {
            QPixmap *pix = new QPixmap(":/defend/tortoise3.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        ui->s1_name->setText("贝尔");
        ui->s1_hp->setText(QString("%1/%2").arg(tortoise(userLevel).getfight_HP()).arg(tortoise(userLevel).getHP()));
    }
    else if(userName == "火炎贝")
    {
        if(userLevel < 5)
        {
            QPixmap *pix = new QPixmap(":/defend/shell1.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        else if(5 <= userLevel && userLevel <10)
        {
            QPixmap *pix = new QPixmap(":/defend/shell2.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        else if(userLevel >= 10 && userLevel <= 15)
        {
            QPixmap *pix = new QPixmap(":/defend/shell3.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        ui->s1_name->setText("火炎贝");
        ui->s1_hp->setText(QString("%1/%2").arg(shell(userLevel).getfight_HP()).arg(shell(userLevel).getHP()));
    }
    else if(userName == "布布种子")
    {
        if(userLevel < 5)
        {
            QPixmap *pix = new QPixmap(":/hp/seed1.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        else if(5 <= userLevel && userLevel <10)
        {
            QPixmap *pix = new QPixmap(":/hp/seed2.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        else if(userLevel >= 10 && userLevel <= 15)
        {
            QPixmap *pix = new QPixmap(":/hp/seed3.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        ui->s1_name->setText("布布种子");
        ui->s1_hp->setText(QString("%1/%2").arg(seed(userLevel).getfight_HP()).arg(seed(userLevel).getHP()));
    }
    else if(userName == "伊修")
    {
        if(userLevel < 5)
        {
            QPixmap *pix = new QPixmap(":/hp/undine1.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        else if(5 <= userLevel && userLevel <10)
        {
            QPixmap *pix = new QPixmap(":/hp/undine2.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        else if(userLevel >= 10 && userLevel <= 15)
        {
            QPixmap *pix = new QPixmap(":/hp/undine3.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        ui->s1_name->setText("伊修");
        ui->s1_hp->setText(QString("%1/%2").arg(undine(userLevel).getfight_HP()).arg(undine(userLevel).getHP()));
    }
    else if(userName == "悠悠")
    {
        if(userLevel < 5)
        {
            QPixmap *pix = new QPixmap(":/speed/bat1.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        else if(5 <= userLevel && userLevel <10)
        {
            QPixmap *pix = new QPixmap(":/speed/bat2.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        else if(userLevel >= 10 && userLevel <= 15)
        {
            QPixmap *pix = new QPixmap(":/speed/bat3.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        ui->s1_name->setText("悠悠");
        ui->s1_hp->setText(QString("%1/%2").arg(bat(userLevel).getfight_HP()).arg(bat(userLevel).getHP()));
    }
    else if(userName == "比比鼠")
    {
        if(userLevel < 5)
        {
            QPixmap *pix = new QPixmap(":/speed/mouse1.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        else if(5 <= userLevel && userLevel <10)
        {
            QPixmap *pix = new QPixmap(":/speed/mouse2.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        else if(userLevel >= 10 && userLevel <= 15)
        {
            QPixmap *pix = new QPixmap(":/speed/mouse3.svg");
            QSize sz = ui->spirit_1->size();
            ui->spirit_1->setPixmap(pix->scaled(sz));
        }
        ui->s1_name->setText("比比鼠");
        ui->s1_hp->setText(QString("%1/%2").arg(mouse(userLevel).getfight_HP()).arg(mouse(userLevel).getHP()));
    }
    //敌方精灵
    if(enemyName == "炎火猴")
    {
        if(enemyLevel < 5)
        {
            QPixmap *pix = new QPixmap(":/attack/FireMonkey1.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        else if(5 <= enemyLevel && enemyLevel <10)
        {
            QPixmap *pix = new QPixmap(":/attack/FireMonkey2.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        else if(enemyLevel >= 10 && enemyLevel <= 15)
        {
            QPixmap *pix = new QPixmap(":/attack/FireMonkey3.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        ui->s2_name->setText("炎火猴");
        ui->s2_hp->setText(QString("%1/%2").arg(FireMonkey(enemyLevel).getfight_HP()).arg(FireMonkey(enemyLevel).getHP()));
        s2 = new FireMonkey(enemyLevel);
    }
    else if(enemyName == "仙人球")
    {
        if(enemyLevel < 5)
        {
            QPixmap *pix = new QPixmap(":/attack/Cactus1.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        else if(5 <= enemyLevel && enemyLevel <10)
        {
            QPixmap *pix = new QPixmap(":/attack/Cactus2.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        else if(enemyLevel >= 10 && enemyLevel <= 15)
        {
            QPixmap *pix = new QPixmap(":/attack/Cactus3.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        ui->s2_name->setText("仙人球");
        ui->s2_hp->setText(QString("%1/%2").arg(Cactus(enemyLevel).getfight_HP()).arg(Cactus(enemyLevel).getHP()));
        s2 = new Cactus(enemyLevel);
    }
    else if(enemyName == "贝尔")
    {
        if(enemyLevel < 5)
        {
            QPixmap *pix = new QPixmap(":/defend/tortoise1.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        else if(5 <= enemyLevel && enemyLevel <10)
        {
            QPixmap *pix = new QPixmap(":/defend/tortoise2.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        else if(enemyLevel >= 10 && enemyLevel <= 15)
        {
            QPixmap *pix = new QPixmap(":/defend/tortoise3.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        ui->s2_name->setText("贝尔");
        ui->s2_hp->setText(QString("%1/%2").arg(tortoise(enemyLevel).getfight_HP()).arg(tortoise(enemyLevel).getHP()));
        s2 = new tortoise(enemyLevel);
    }
    else if(enemyName == "火炎贝")
    {
        if(enemyLevel < 5)
        {
            QPixmap *pix = new QPixmap(":/defend/shell1.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        else if(5 <= enemyLevel && enemyLevel <10)
        {
            QPixmap *pix = new QPixmap(":/defend/shell2.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        else if(enemyLevel >= 10 && enemyLevel <= 15)
        {
            QPixmap *pix = new QPixmap(":/defend/shell3.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        ui->s2_name->setText("火炎贝");
        ui->s2_hp->setText(QString("%1/%2").arg(shell(enemyLevel).getfight_HP()).arg(shell(enemyLevel).getHP()));
        s2 = new shell(enemyLevel);
    }
    else if(enemyName == "布布种子")
    {
        if(enemyLevel < 5)
        {
            QPixmap *pix = new QPixmap(":/hp/seed1.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        else if(5 <= enemyLevel && enemyLevel <10)
        {
            QPixmap *pix = new QPixmap(":/hp/seed2.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        else if(enemyLevel >= 10 && enemyLevel <= 15)
        {
            QPixmap *pix = new QPixmap(":/hp/seed3.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        ui->s2_name->setText("布布种子");
        ui->s2_hp->setText(QString("%1/%2").arg(seed(enemyLevel).getfight_HP()).arg(seed(enemyLevel).getHP()));
        s2 = new seed(enemyLevel);
    }
    else if(enemyName == "伊修")
    {
        if(enemyLevel < 5)
        {
            QPixmap *pix = new QPixmap(":/hp/undine1.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        else if(5 <= enemyLevel && enemyLevel <10)
        {
            QPixmap *pix = new QPixmap(":/hp/undine2.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        else if(enemyLevel >= 10 && enemyLevel <= 15)
        {
            QPixmap *pix = new QPixmap(":/hp/undine3.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        ui->s2_name->setText("伊修");
        ui->s2_hp->setText(QString("%1/%2").arg(undine(enemyLevel).getfight_HP()).arg(undine(enemyLevel).getHP()));
        s2 = new undine(enemyLevel);
    }
    else if(enemyName == "悠悠")
    {
        if(enemyLevel < 5)
        {
            QPixmap *pix = new QPixmap(":/speed/bat1.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        else if(5 <= enemyLevel && enemyLevel <10)
        {
            QPixmap *pix = new QPixmap(":/speed/bat2.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        else if(enemyLevel >= 10 && enemyLevel <= 15)
        {
            QPixmap *pix = new QPixmap(":/speed/bat3.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        ui->s2_name->setText("悠悠");
        ui->s2_hp->setText(QString("%1/%2").arg(bat(enemyLevel).getfight_HP()).arg(bat(enemyLevel).getHP()));
        s2 = new bat(enemyLevel);
    }
    else if(enemyName == "比比鼠")
    {
        if(enemyLevel < 5)
        {
            QPixmap *pix = new QPixmap(":/speed/mouse1.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        else if(5 <= enemyLevel && enemyLevel <10)
        {
            QPixmap *pix = new QPixmap(":/speed/mouse2.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        else if(enemyLevel >= 10 && enemyLevel <= 15)
        {
            QPixmap *pix = new QPixmap(":/speed/mouse3.svg");
            QSize sz = ui->spirit_2->size();
            QPixmap mirroredPix = pix->transformed(QTransform().scale(-1, 1));  // 水平镜像
            ui->spirit_2->setPixmap(mirroredPix.scaled(sz));
        }
        ui->s2_name->setText("比比鼠");
        ui->s2_hp->setText(QString("%1/%2").arg(mouse(enemyLevel).getfight_HP()).arg(mouse(enemyLevel).getHP()));
        s2 = new mouse(enemyLevel);
    }
    // 切换到战斗页面
    ui->result->hide();
    s1->setID(ID);
    ui->stackedWidget->setCurrentWidget(ui->fight_page);
    battle(s1, s2);
    soundEffect->stop();
    QString soundFilePath;
    // 根据页面索引设置不同的音频文件路径
    soundFilePath = ":/music/fight_bgm.wav"; // 确保文件路径正确

    if (!soundFilePath.isEmpty()) {
        soundEffect = new QSoundEffect(this);
        soundEffect->setSource(QUrl::fromLocalFile(soundFilePath));
        soundEffect->setLoopCount(QSoundEffect::Infinite);//设置循环播放
        soundEffect->setVolume(0.8); // 设置音量
        soundEffect->play(); // 播放音频
    }

}
//开始战斗按钮
void Lobby::updatePlayerHP(int currentHP, int maxHP)
{
    if(currentHP <0)
        currentHP = 0;
    ui->s1_hp->setText(QString("%1/%2").arg(currentHP).arg(maxHP));
    ui->s1_hp_bar->setMaximum(maxHP);
    ui->s1_hp_bar->setValue(currentHP);
}

void Lobby::updateEnemyHP(int currentHP, int maxHP)
{   if(currentHP <0)
        currentHP = 0;
    ui->s2_hp->setText(QString("%1/%2").arg(currentHP).arg(maxHP));
    ui->s2_hp_bar->setMaximum(maxHP);
    ui->s2_hp_bar->setValue(currentHP);
}

void Lobby::battle(pet *s1, pet *s2)
{
    ui->log->clear();
    updatePlayerHP(s1->getfight_HP(),s1->getHP());
    updateEnemyHP(s2->getfight_HP(),s2->getHP());
    userTimer = new QTimer(this);
    enemyTimer = new QTimer(this);
    //连接计时器
    connect(userTimer, &QTimer::timeout, this, &Lobby::onUserAttack);
    connect(enemyTimer, &QTimer::timeout, this, &Lobby::onEnemyAttack);
    //连接日志
    connect(s1, &pet::log, this, [this](const QString &message) {
        this->appendToLog(message);
    });
    connect(s2, &pet::log, this, [this](const QString &message) {
        this->appendToLog(message);
    });

    userTimer->start(static_cast<int>(s1->getAttackInterval() * 1000));
    enemyTimer->start(static_cast<int>(s2->getAttackInterval() * 1000));

}

void Lobby::onUserAttack()
{
    if (!s1 || !s2) return;

    QString logMessage = QString("<span style='color:green;'>%1</span>攻击<span style='color:red;'>%2</span>").arg(s1->getName()).arg(s2->getName());
    ui->log->append(logMessage); // 发射 logMessage 信号
    // 判断是否暴击
    bool criticalHit = false;
    if (rand() % 100 < 20) {
        criticalHit = true;
        QString logMessage = QString("暴击！");
        ui->log->append(logMessage); // 发射 logMessage 信号
    }

    // 判断是否闪避
    bool dodge = false;
    if (rand() % 100 < 10) {
        dodge = true;
        logMessage = QString("<span style='color:red;'>%1</span>闪避了攻击！").arg(s2->getName());
        ui->log->append(logMessage); // 发射 logMessage 信号
    }

    if (!dodge) {
        if (criticalHit) {
            // 暴击伤害加成
            s1->attack(s2, true);
        } else {
            s1->attack(s2, false);
        }
        updateEnemyHP(s2->getfight_HP(),s2->getHP());
        checkBattleStatus();
        // 创建一个新的QPropertyAnimation实例
        QPropertyAnimation *animation = new QPropertyAnimation(ui->spirit_2, "geometry");
        animation->setDuration(200); // 持续时间更短

        QRect startRect = ui->spirit_2->geometry();
        animation->setKeyValueAt(0, startRect);
        animation->setKeyValueAt(0.1, QRect(startRect.x() - 5, startRect.y(), startRect.width(), startRect.height()));
        animation->setKeyValueAt(0.2, startRect);
        animation->setKeyValueAt(0.3, QRect(startRect.x() + 5, startRect.y(), startRect.width(), startRect.height()));
        animation->setKeyValueAt(0.4, startRect);
        animation->setKeyValueAt(0.5, QRect(startRect.x() - 5, startRect.y(), startRect.width(), startRect.height()));
        animation->setKeyValueAt(0.6, startRect);
        animation->setKeyValueAt(0.7, QRect(startRect.x() + 5, startRect.y(), startRect.width(), startRect.height()));
        animation->setKeyValueAt(0.8, startRect);
        animation->setKeyValueAt(0.9, QRect(startRect.x() - 5, startRect.y(), startRect.width(), startRect.height()));
        animation->setKeyValueAt(1, startRect);

        animation->setEasingCurve(QEasingCurve::InOutCubic);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }

}

void Lobby::onEnemyAttack()
{
    if (!s1 || !s2) return;

    QString logMessage = QString("<span style='color:red;'>%1</span>攻击<span style='color:green;'>%2</span>").arg(s2->getName()).arg(s1->getName());
    ui->log->append(logMessage); // 发射 logMessage 信号
    // 判断是否暴击
    bool criticalHit = false;
    if (rand() % 100 < 20) {
        criticalHit = true;
        QString logMessage = QString("暴击！");
        ui->log->append(logMessage); // 发射 logMessage 信号
    }

    // 判断是否闪避
    bool dodge = false;
    if (rand() % 100 < 10) {
        dodge = true;
        qDebug() << s1->getName() << "闪避了攻击！";
        logMessage = QString("<span style='color:green;'>%1</span>闪避了攻击！").arg(s1->getName());
        ui->log->append(logMessage); // 发射 logMessage 信号
    }

    if (!dodge) {
        if (criticalHit) {
            // 暴击伤害加成
            s2->attack(s1, true);
        } else {
            s2->attack(s1, false);
        }
        updatePlayerHP(s1->getfight_HP(),s1->getHP());
        checkBattleStatus();
        QPropertyAnimation *animation = new QPropertyAnimation(ui->spirit_1, "geometry");
        animation->setDuration(200); // 持续时间更短

        QRect startRect = ui->spirit_1->geometry();
        animation->setKeyValueAt(0, startRect);
        animation->setKeyValueAt(0.1, QRect(startRect.x() + 10, startRect.y(), startRect.width(), startRect.height()));
        animation->setKeyValueAt(0.2, startRect);
        animation->setKeyValueAt(0.3, QRect(startRect.x() - 10, startRect.y(), startRect.width(), startRect.height()));
        animation->setKeyValueAt(0.4, startRect);
        animation->setKeyValueAt(0.5, QRect(startRect.x() + 10, startRect.y(), startRect.width(), startRect.height()));
        animation->setKeyValueAt(0.6, startRect);
        animation->setKeyValueAt(0.7, QRect(startRect.x() - 10, startRect.y(), startRect.width(), startRect.height()));
        animation->setKeyValueAt(0.8, startRect);
        animation->setKeyValueAt(0.9, QRect(startRect.x() + 10, startRect.y(), startRect.width(), startRect.height()));
        animation->setKeyValueAt(1, startRect);

        animation->setEasingCurve(QEasingCurve::InOutCubic);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }

}

void Lobby::checkBattleStatus()
{
    QString logMessage;
    if (s1->isDead() || s2->isDead()) {
        userTimer->stop();
        enemyTimer->stop();

        if (s1->isDead() && s2->isDead()) {
            logMessage = QString("战斗结束！双方同时倒下，平局！");
            ui->log->append(logMessage); // 发射 logMessage 信号
            Draw(s1);
        } else if (s1->isDead()) {
            logMessage = QString("战斗结束！<span style='color:green;'>%1</span>被击败！").arg(s1->getName());
            ui->log->append(logMessage); // 发射 logMessage 信号
            Lost(s1);
        } else if (s2->isDead()) {
            logMessage = QString("战斗结束！<span style='color:red;'>%1</span>被击败！").arg(s2->getName());
            ui->log->append(logMessage); // 发射 logMessage 信号
            Win(s1,s2);

        }
    }
}
//确认战斗结果
void Lobby::Draw(pet *s1)
{
    ui->result->show();
    ui->result_label->setText("平局");
    int exp = 500;
    int _level = s1->getLevel() - s2->getLevel();
    if(_level == 0)
        exp = 500;
    else if(_level < 0)
        exp = exp * (1 - _level);
    else if(_level > 0)
        exp = exp/(1 + _level);
    s1->setExp(s1->getExperience() + exp);
    ui->exp_label->setText(QString("精灵获得%1点经验，获得200金币").arg(exp));
    while((s1->getExperience() > (s1->getLevel() * 500)) && s1->getLevel() < 15)
    {
        s1->setExp(s1->getExperience() - (s1->getLevel() * 500));
        s1->setlevel(s1->getLevel() + 1);
        s1->levelUp();
    }
    QJsonObject spirit;
    spirit["spirit_ID"] = s1->getID();
    spirit["user_ID"] = manage.userID;
    spirit["evolution"] = s1->getEvolution();
    spirit["spirit_name"] = s1->getName();
    spirit["level"] = s1->getLevel();
    spirit["experience"] = s1->getExperience();
    spirit["attack"] = s1->getAttack();
    spirit["hp"] = s1->getHP();
    spirit["defense"] = s1->getDefense();
    spirit["attackInterval"] = s1->getAttackInterval();

    QJsonObject json  = {
        {"kind", EXP},
        {"socketID", manage.socketID},
        {"user_ID", manage.userID},
        {"spirit", spirit},
        {"drug", manage.drug}
    };
    socket->write(QString(QJsonDocument(json).toJson()).toUtf8().data());
}//平局函数，无失败惩罚，基础经验奖励升为500

void Lobby::Lost(pet *s1)
{
    ui->result->show();
    ui->result_label->setText("失败");
    int exp = 180;
    s1->setExp(s1->getExperience() + exp);
    ui->exp_label->setText(QString("精灵获得%1点经验，获得200金币").arg(exp));
    while((s1->getExperience() > (s1->getLevel() * 500)) && s1->getLevel() < 15)
    {
        s1->setExp(s1->getExperience() - (s1->getLevel() * 500));
        s1->setlevel(s1->getLevel() + 1);
        s1->levelUp();
    }
    QJsonObject spirit;
    spirit["spirit_ID"] = s1->getID();
    spirit["user_ID"] = manage.userID;
    spirit["evolution"] = s1->getEvolution();
    spirit["spirit_name"] = s1->getName();
    spirit["level"] = s1->getLevel();
    spirit["experience"] = s1->getExperience();
    spirit["attack"] = s1->getAttack();
    spirit["hp"] = s1->getHP();
    spirit["defense"] = s1->getDefense();
    spirit["attackInterval"] = s1->getAttackInterval();

    QJsonObject json  = {
        {"kind", EXP},
        {"socketID", manage.socketID},
        {"user_ID", manage.userID},
        {"spirit", spirit},
        {"drug", manage.drug}
    };
    socket->write(QString(QJsonDocument(json).toJson()).toUtf8().data());//更新精灵信息
    if(fight_type == 1)
    {
        // 使用 QTimer::singleShot 在延时后发送第二条消息
        QTimer::singleShot(500, this, [=]() {
            updateResultPage(spiritsArray);
            ui->stackedWidget->setCurrentWidget(ui->result_page);
        });
    }
}//失败函数

void Lobby::Win(pet *s1, pet *s2)
{
    ui->result->show();
    ui->result_label->setText("胜利！");
    int exp = 360;//打赢一场给360
    int _level = s1->getLevel() - s2->getLevel();
    if(_level == 0)
        exp = 360;
    else if(_level < 0)
        exp = exp * (1 - _level);
    else if(_level > 0)
        exp = exp/(1 + _level);
    s1->setExp(s1->getExperience() + exp);
    ui->exp_label->setText(QString("精灵获得%1点经验，获得500金币").arg(exp));
    while((s1->getExperience() > (s1->getLevel() * 500)) && s1->getLevel() < 15)
    {
        s1->setExp(s1->getExperience() - (s1->getLevel() * 500));
        s1->setlevel(s1->getLevel() + 1);
        s1->levelUp();
    }
    QJsonObject spirit;
    spirit["spirit_ID"] = s1->getID();
    spirit["user_ID"] = manage.userID;
    spirit["evolution"] = s1->getEvolution();
    spirit["spirit_name"] = s1->getName();
    spirit["level"] = s1->getLevel();
    spirit["experience"] = s1->getExperience();
    spirit["attack"] = s1->getAttack();
    spirit["hp"] = s1->getHP();
    spirit["defense"] = s1->getDefense();
    spirit["attackInterval"] = s1->getAttackInterval();

    QJsonObject json  = {
        {"kind", EXP_W},
        {"socketID", manage.socketID},
        {"user_ID", manage.userID},
        {"spirit", spirit},
        {"drug", manage.drug}
    };
    socket->write(QString(QJsonDocument(json).toJson()).toUtf8().data());
    if(fight_type == 1)
    {
        // 使用 QTimer::singleShot 在延时后发送第二条消息
        QTimer::singleShot(500, this, [=]() {
            // 构建第二个消息的 JSON 对象
            QJsonObject spirit2;
            spirit2["user_ID"] = manage.userID;
            spirit2["evolution"] = s2->getEvolution();
            spirit2["spirit_name"] = s2->getName();
            spirit2["level"] = s2->getLevel();
            spirit2["experience"] = s2->getExperience();
            spirit2["attack"] = s2->getAttack();
            spirit2["hp"] = s2->getHP();
            spirit2["defense"] = s2->getDefense();
            spirit2["attackInterval"] = s2->getAttackInterval();

            QJsonObject json2 = {
                {"kind", WIN},
                {"socketID", manage.socketID},
                {"user_ID", manage.userID},
                {"spirit", spirit2}
            };

            // 发送第二条消息
            socket->write(QString(QJsonDocument(json2).toJson()).toUtf8().data());
        });
    }

}//胜利函数

void Lobby::updateResultPage(const QJsonArray &spirits)
{
    QGridLayout *layout = new QGridLayout; // 使用网格布局

    // 随机选择三只精灵
    QJsonArray randomSpirits;
    QSet<int> selectedIndexes;
    int numSpiritsToSelect = qMin(3, spirits.size());
    while (selectedIndexes.size() < numSpiritsToSelect) {
        int index = QRandomGenerator::global()->bounded(spirits.size());
        if (!selectedIndexes.contains(index)) {
            selectedIndexes.insert(index);
            randomSpirits.append(spirits[index].toObject());
        }
    }

    for (int i = 0; i < randomSpirits.size(); ++i) {
        QJsonObject spirit = randomSpirits[i].toObject();
        QString name = spirit["spirit_name"].toString();
        int level = spirit["level"].toInt();
        QString imagePath;

        imagePath = getImagePath(name, level);

        // 创建一个按钮显示精灵信息
        QPushButton *button = new QPushButton;
        button->setIcon(QIcon(imagePath));
        button->setIconSize(QSize(120, 120));
        button->setText(QString("%1 (Level: %2)").arg(name).arg(level));
        button->setObjectName(QString::number(spirit["spirit_ID"].toInt()));

        // 连接按钮点击信号到槽函数
        connect(button, &QPushButton::clicked, [this, spirit]() {
            int spiritID = spirit["spirit_ID"].toInt();
            // 在这里处理选中的精灵 ID
            QJsonObject json = {
                {"kind", LOST},
                {"socketID", manage.socketID},
                {"username", manage.username},
                {"user_ID", manage.userID},
                {"spirit_ID", spiritID}
            };
            socket->write(QString(QJsonDocument(json).toJson()).toUtf8().data());
            ui->stackedWidget->setCurrentWidget(ui->fight_page);
        });

        layout->addWidget(button, i / 3, i % 3); // 将按钮添加到网格布局
    }

    QWidget *resultPage = ui->result_page;
    QLayout *oldLayout = resultPage->layout();
    if (oldLayout) {
        QLayoutItem *item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete oldLayout;
    }

    resultPage->setLayout(layout);
}//随机挑选3只精灵

void Lobby::on_fail_button_clicked()
{
    userTimer->stop();
    enemyTimer->stop();
    QString logMessage;
    logMessage = QString("战斗结束！%1认输").arg(s1->getName());
    ui->log->append(logMessage); // 发射 logMessage 信号
    int temp = fight_type;
    fight_type = 0;
    Lost(s1);
    fight_type = temp;
}
//认输
void Lobby::appendToLog(const QString &message)
{
    ui->log->append(message); // 假设 ui 是指向 fight_page 界面的指针
}
//发送战斗信息到日志

void Lobby::on_tab_host_tabBarClicked(int index)
{
    fight_type = index;
}
//确定战斗种类

void Lobby::on_pushButton_clicked()
{
    ui->result->hide();
    ui->stackedWidget->setCurrentWidget(ui->lobby_page);
    QJsonObject json  = {
        {"kind", LOBBY},
        {"socketID", manage.socketID},
        {"username", manage.username},
        {"user_ID", manage.userID}
    };
    socket->write(QString(QJsonDocument(json).toJson()).toUtf8().data());//进入就获取用户精灵数据
    soundEffect->stop();
    QString soundFilePath;
    // 根据页面索引设置不同的音频文件路径
    soundFilePath = ":/music/kannong.wav"; // 确保文件路径正确

    if (!soundFilePath.isEmpty()) {
        soundEffect = new QSoundEffect(this);
        soundEffect->setSource(QUrl::fromLocalFile(soundFilePath));
        soundEffect->setLoopCount(QSoundEffect::Infinite);//设置循环播放
        soundEffect->setVolume(0.5); // 设置音量
        soundEffect->play(); // 播放音频
    }
}
//从战斗结果处返回大厅
QString Lobby::getImagePath(const QString &name, int level) const {
    QString imagePath;

    if (name == "炎火猴") {
        if (level < 5) imagePath = ":/attack/FireMonkey1.svg";
        else if (level < 10) imagePath = ":/attack/FireMonkey2.svg";
        else imagePath = ":/attack/FireMonkey3.svg";
    } else if (name == "仙人球") {
        if (level < 5) imagePath = ":/attack/Cactus1.svg";
        else if (level < 10) imagePath = ":/attack/Cactus2.svg";
        else imagePath = ":/attack/Cactus3.svg";
    } else if (name == "贝尔") {
        if (level < 5) imagePath = ":/defend/tortoise1.svg";
        else if (level < 10) imagePath = ":/defend/tortoise2.svg";
        else imagePath = ":/defend/tortoise3.svg";
    } else if (name == "火炎贝") {
        if (level < 5) imagePath = ":/defend/shell1.svg";
        else if (level < 10) imagePath = ":/defend/shell2.svg";
        else imagePath = ":/defend/shell3.svg";
    } else if (name == "布布种子") {
        if (level < 5) imagePath = ":/hp/seed1.svg";
        else if (level < 10) imagePath = ":/hp/seed2.svg";
        else imagePath = ":/hp/seed3.svg";
    } else if (name == "伊修") {
        if (level < 5) imagePath = ":/hp/undine1.svg";
        else if (level < 10) imagePath = ":/hp/undine2.svg";
        else imagePath = ":/hp/undine3.svg";
    } else if (name == "悠悠") {
        if (level < 5) imagePath = ":/speed/bat1.svg";
        else if (level < 10) imagePath = ":/speed/bat2.svg";
        else imagePath = ":/speed/bat3.svg";
    } else if (name == "比比鼠") {
        if (level < 5) imagePath = ":/speed/mouse1.svg";
        else if (level < 10) imagePath = ":/speed/mouse2.svg";
        else imagePath = ":/speed/mouse3.svg";
    }

    return imagePath;
}

void Lobby::on_other_users_2_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->userlist);
    QJsonObject json  = {
        {"kind", USERLIST},
        {"socketID", manage.socketID},
        {"user_ID", manage.userID},
    };
    socket->write(QString(QJsonDocument(json).toJson()).toUtf8().data());
}
//转换到其他用户列表
void Lobby::Userlist( QJsonObject json) {
    // 确保仅连接一次信号和槽
    static bool isConnected = false;
    if (!isConnected) {
        connect(ui->userlistTableWidget, &QTableWidget::itemDoubleClicked, this, &Lobby::on_userlistTableWidget_itemDoubleClicked);
        isConnected = true;
    }
    QJsonArray usersArray = json["users"].toArray();
    qDebug()<<"test:"<<usersArray<<Qt::endl;
    // 清空 userlist 页面上的内容
    ui->userlistTableWidget->clearContents();
    ui->userlistTableWidget->setRowCount(usersArray.size());
    ui->userlistTableWidget->setColumnCount(8); // 根据显示的信息数量调整列数

    QStringList headers = {"用户名", "在线状态", "精灵数", "胜利场数", "总场数", "胜率", "宠物勋章", "高级勋章"};
    ui->userlistTableWidget->setHorizontalHeaderLabels(headers);

    // 设置只能选择整行
    ui->userlistTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->userlistTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 使列宽度自动调整以填满表格
    ui->userlistTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    int row = 0;
    for (const QJsonValue& value : usersArray) {
        QJsonObject userInfo = value.toObject();
        QString userName = userInfo["user_name"].toString();
        int online = userInfo["online"].toInt();
        int petNum = userInfo["pet_num"].toInt();
        int victoryCount = userInfo["victory_count"].toInt();
        int totalGames = userInfo["total_games"].toInt();
        double winRatio = userInfo["win_ratio"].toDouble();
        int medal1 = userInfo["medal1"].toInt();
        int medal2 = userInfo["medal2"].toInt();

        QTableWidgetItem* itemUserName = new QTableWidgetItem(userName);
        itemUserName->setFlags(itemUserName->flags() & ~Qt::ItemIsEditable);
        ui->userlistTableWidget->setItem(row, 0, itemUserName);
        QTableWidgetItem* itemOnline;
        if(online == 0)
            itemOnline = new QTableWidgetItem("离线");
        else
            itemOnline = new QTableWidgetItem("在线");

        itemOnline->setFlags(itemOnline->flags() & ~Qt::ItemIsEditable);
        ui->userlistTableWidget->setItem(row, 1, itemOnline);

        QTableWidgetItem* itemPetNum = new QTableWidgetItem(QString::number(petNum));
        itemPetNum->setFlags(itemPetNum->flags() & ~Qt::ItemIsEditable);
        ui->userlistTableWidget->setItem(row, 2, itemPetNum);

        QTableWidgetItem* itemVictoryCount = new QTableWidgetItem(QString::number(victoryCount));
        itemVictoryCount->setFlags(itemVictoryCount->flags() & ~Qt::ItemIsEditable);
        ui->userlistTableWidget->setItem(row, 3, itemVictoryCount);

        QTableWidgetItem* itemTotalGames = new QTableWidgetItem(QString::number(totalGames));
        itemTotalGames->setFlags(itemTotalGames->flags() & ~Qt::ItemIsEditable);
        ui->userlistTableWidget->setItem(row, 4, itemTotalGames);

        QTableWidgetItem* itemWinRatio = new QTableWidgetItem(QString::number(winRatio));
        itemWinRatio->setFlags(itemWinRatio->flags() & ~Qt::ItemIsEditable);
        ui->userlistTableWidget->setItem(row, 5, itemWinRatio);

        // 设置 medal1 的显示文本
        QString medal1Text;
        switch (medal1) {
        case 0: medal1Text = "新人"; break;
        case 1: medal1Text = "青铜"; break;
        case 2: medal1Text = "白银"; break;
        case 3: medal1Text = "黄金"; break;
        default: medal1Text = "未知"; break;
        }

        QTableWidgetItem* itemMedal1 = new QTableWidgetItem(medal1Text);
        itemMedal1->setFlags(itemMedal1->flags() & ~Qt::ItemIsEditable);
        ui->userlistTableWidget->setItem(row, 6, itemMedal1);

        // 设置 medal2 的显示文本
        QString medal2Text;
        switch (medal2) {
        case 0: medal2Text = "无"; break;
        case 1: medal2Text = "青铜"; break;
        case 2: medal2Text = "白银"; break;
        case 3: medal2Text = "黄金"; break;
        default: medal2Text = "未知"; break;
        }

        QTableWidgetItem* itemMedal2 = new QTableWidgetItem(medal2Text);
        itemMedal2->setFlags(itemMedal2->flags() & ~Qt::ItemIsEditable);
        ui->userlistTableWidget->setItem(row, 7, itemMedal2);


        // 将 userInfo 存储在表项的数据中
        ui->userlistTableWidget->item(row, 0)->setData(Qt::UserRole, userInfo);

        row++;
    }

}
//显示其他用户信息

void Lobby::on_userlistTableWidget_itemDoubleClicked(QTableWidgetItem *item) {
    if (spiritDialog) {
        spiritDialog->close();
        delete spiritDialog;
    }

    QTableWidgetItem *firstColumnItem = ui->userlistTableWidget->item(item->row(), 0);
    QJsonObject userInfo = firstColumnItem->data(Qt::UserRole).toJsonObject();
    qDebug() << "User_Info:" << userInfo << Qt::endl;

    QJsonArray spirit_Array = userInfo["spirits"].toArray();
    int petNum = userInfo["pet_num"].toInt();
    qDebug()<<spirit_Array<<petNum<<Qt::endl;
    spiritDialog = new SpiritInfoDialog(spirit_Array, petNum, this);
    spiritDialog->setAttribute(Qt::WA_DeleteOnClose);
    spiritDialog->show();
}
//确认信息被双击转到用户精灵处
void Lobby::on_back_lobby_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->lobby_page);
    QJsonObject json  = {
        {"kind", LOBBY},
        {"socketID", manage.socketID},
        {"username", manage.username},
        {"user_ID", manage.userID}
    };
    socket->write(QString(QJsonDocument(json).toJson()).toUtf8().data());

}
//返回大厅

void Lobby::on_back_lobby_2_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->lobby_page);
    QJsonObject json  = {
        {"kind", LOBBY},
        {"socketID", manage.socketID},
        {"username", manage.username},
        {"user_ID", manage.userID}
    };
    socket->write(QString(QJsonDocument(json).toJson()).toUtf8().data());
}
//返回大厅

void Lobby::on_store_button_2_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->shop_page);
    ui->drug_num->setText(QString::number(manage.drug));
    ui->money_num->setText(QString::number(manage.money));
}
//进入商店

void Lobby::on_buy_clicked()
{
    if(manage.money >= 500){
        manage.money = manage.money - 500;
        manage.drug++;
    }
    else{
        QMessageBox::critical(this, "抱歉", "本店不接受赊账");
    }
    QJsonObject json  = {
        {"kind", BUY},
        {"socketID", manage.socketID},
        {"username", manage.username},
        {"user_ID", manage.userID},
        {"money", manage.money},
        {"drug", manage.drug}
    };
    socket->write(QString(QJsonDocument(json).toJson()).toUtf8().data());
    ui->drug_num->setText(QString::number(manage.drug));
    ui->money_num->setText(QString::number(manage.money));
}
//购买

void Lobby::on_recover_button_clicked()
{
    if(manage.drug > 0){
        manage.drug--;
        ui->now_drug->setText(QString::number(manage.drug));
        if(s1->getfight_HP() + 40 <= s1->getHP())
        {
            s1->setfight_HP(s1->getfight_HP() + 40);
        }
        else
            s1->setfight_HP(s1->getHP());
        updatePlayerHP(s1->getfight_HP(), s1->getHP());
    }
}//使用一次回复40血量

