#include "login.h"
#include "ui_login.h"


Login::Login(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Login)

{
    ui->setupUi(this);
    //设置图片
    QPixmap *pix = new QPixmap(":/new/prefix1/kgqgj_1.png");
    QSize sz = ui->background->size();
    ui->background->setPixmap(pix->scaled(sz));

    //点击登陆

    socket = new QTcpSocket;
    connect(ui->quit, SIGNAL(clicked()), this, SLOT(close()));
    //收到服务器回复
    connect(socket, SIGNAL(readyRead()), this, SLOT(receiveFromServer()));
    socket->connectToHost("127.0.0.1", 13520);

    QString soundFilePath;
    // 根据页面索引设置不同的音频文件路径
    soundFilePath = ":/music/hanlunna.wav"; // 确保文件路径正确

    if (!soundFilePath.isEmpty()) {
        soundEffect = new QSoundEffect(this);
        soundEffect->setSource(QUrl::fromLocalFile(soundFilePath));
        soundEffect->setLoopCount(QSoundEffect::Infinite);//设置循环播放
        soundEffect->setVolume(0.5); // 设置音量
        soundEffect->play(); // 播放音频
    }
}



void Login::on_quit_clicked()
{
    this->close();
}


void Login::reconnect()
{
    bool flag = connect(socket, SIGNAL(readyRead()), this, SLOT(receiveFromServer()));

    if(flag == true)
    {
        qDebug()<<"Login与服务器连接"<<Qt::endl;
    }
    else
    {
        qDebug()<<"Login未能与服务器连接"<<Qt::endl;
    }

}

void Login::receiveFromServer()
{
    //从通信套接字中取出内容
    QByteArray array = socket->readAll();
    QJsonDocument jsonDocument = QJsonDocument::fromJson(array);
    QJsonObject json = jsonDocument.object();
    qDebug()<<"login收到服务器："<<json<<Qt::endl;

    if(json["kind"] == SOCKET){
        if(flag == 0){
            manage.socketID = json["socketID"].toInt();
            flag = 1;
        }
    }

    if(json["socketID"].toInt() != manage.socketID)
    {
        return;
    }
    //密码正确
    if(json["kind"] == LOGIN_SUCCEED)
    {
        qDebug() << "进入游戏大厅" << Qt::endl;
        manage.username = json["username"].toString();
        manage.userID = json["userID"].toInt();
        qDebug()<<"ma.username:"<<manage.username<<Qt::endl;
        qDebug()<<"Login释放与服务器连接"<<Qt::endl;
        // 创建新的 socket 对象用于大厅的连接
        QTcpSocket *lobbySocket = new QTcpSocket();
        lobbySocket->connectToHost("127.0.0.1", 13520);

        socket->disconnect(this);//解除连接
        soundEffect->stop();
        // 创建大厅窗口
        lobby = new Lobby(lobbySocket);
        this->close();
        lobby->show();
        connect(lobby, &Lobby::loggedOut, this, &Login::openLoginPage);
    }
    //用户名不存在
    else if(json["kind"] == LOGIN_UNEXISTED){
        QMessageBox::critical(this,"错误","用户名不存在");
    }
    //密码错误
    else if(json["kind"] == LOGIN_FAILED)
    {
        QMessageBox::critical(this, "错误", "密码错误，请重新输入");
    }
    //用户名重复
    else if(json["kind"] == REGISTER_FAILED)
    {
        QMessageBox::critical(this, "错误", "用户名已存在");

    }
    else if(json["kind"] == LOGIN_ONLINE)
    {
        QMessageBox::critical(this,"错误","用户已在线");
    }
    //注册成功
    else if(json["kind"] == REGISTER_SUCCEED)
    {
        QMessageBox::information(this, "提示", "注册成功");
    }

}


Login::~Login()
{
    delete ui;
}


void Login::on_log_in_clicked()
{
    QString name = ui->username->text().toUtf8().data();//获取用户名
    QString passWord = ui->password->text();//获取密码
    if(name == "")
    {
        QMessageBox::critical(this, "错误", "请输入用户名");
        return;
    }
    //如果未输入密码，
    if(passWord == "")
    {
        QMessageBox::critical(this, "错误", "请输入密码");
        return;
    }
    QJsonObject json  = {
        {"kind", LOGIN},
        {"socketID", manage.socketID},
        {"username", name},
        {"password", passWord}
    };
    //向服务器发送信息
    socket->write(QString(QJsonDocument(json).toJson()).toUtf8().data());
    socket->flush(); // 确保数据已发送
    qDebug()<<json<<Qt::endl;
}

void Login::on_register_2_clicked()
{
    QString name = ui->username->text().toUtf8().data();//获取用户名
    QString passWord = ui->password->text();//获取密码
    //如果未输入用户名
    if(name == "")
    {
        QMessageBox::critical(this, "错误", "请输入用户名");
        return;
    }
    //如果未输入密码，
    if(passWord == "")
    {
        QMessageBox::critical(this, "错误", "请输入密码");
        return;
    }
    QJsonObject json  = {
        {"kind", REGISTER},
        {"socketID", manage.socketID},
        {"username", name},
        {"password", passWord}
    };

    socket->write(QString(QJsonDocument(json).toJson()).toUtf8().data());
    socket->flush(); // 确保数据已发送
    qDebug()<<json<<Qt::endl;
}

void Login::openLoginPage()
{
    this->show(); // 显示登录页面
    soundEffect->stop();
    QString soundFilePath;
    // 根据页面索引设置不同的音频文件路径
    soundFilePath = ":/music/hanlunna.wav"; // 确保文件路径正确

    if (!soundFilePath.isEmpty()) {
        soundEffect = new QSoundEffect(this);
        soundEffect->setSource(QUrl::fromLocalFile(soundFilePath));
        soundEffect->setLoopCount(QSoundEffect::Infinite);//设置循环播放
        soundEffect->setVolume(0.5); // 设置音量
        soundEffect->play(); // 播放音频
    }
    // 确保之前的连接已经断开
    disconnect(ui->log_in, SIGNAL(clicked()), this, SLOT(on_log_in_clicked()));
    disconnect(ui->register_2, SIGNAL(clicked()), this, SLOT(on_register_2_clicked()));
    disconnect(socket, SIGNAL(readyRead()), this, SLOT(receiveFromServer()));

    // 重新连接信号到槽
    connect(ui->log_in, SIGNAL(clicked()), this, SLOT(on_log_in_clicked()));
    connect(ui->register_2, SIGNAL(clicked()), this, SLOT(on_register_2_clicked()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(receiveFromServer()));

    // 检查套接字状态并重新连接
    if(socket->state() == QAbstractSocket::UnconnectedState)
    {
        socket->connectToHost("127.0.0.1", 13520);
    }
    else
    {
        // 如果已经连接，清理套接字缓冲区
        socket->readAll();
    }

    // 重置用户名和密码字段
    ui->username->setText("");
    ui->password->setText("");
}



