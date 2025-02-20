#include "pet.h"
#include "qdebug.h"

pet::pet() :QObject(){
    m_level = 1;
    m_experience = 0;
    m_attack = 8;
    m_defense = 5;
    m_hp = 100;
    m_attackInterval = 2.5;
    fight_hp = m_hp;
    fight_hp = m_hp;
    fight_attack = m_attack;
    fight_attackInterval = m_attackInterval;
    fight_defense = m_defense; 
    m_evolution = Normal;
}
std::vector<QString> pet::names = {"炎火猴", "仙人球", "贝尔", "火炎贝", "布布种子", "伊修", "比比鼠", "悠悠"};

pet* pet::createRandomPet(int level) {
    int nameIndex = rand() % names.size();
    QString name = names[nameIndex];
    if (name == "炎火猴")
        return new FireMonkey(level);
    else if (name == "仙人球")
        return new Cactus(level);
    else if(name == "贝尔")
        return new tortoise(level);
    else if(name == "火炎贝")
        return new shell(level);
    else if(name == "布布种子")
        return new seed(level);
    else if(name == "伊修")
        return new undine(level);
    else if(name == "悠悠")
        return new bat(level);
    else if(name == "比比鼠")
        return new mouse(level);
    else
        qDebug()<<"匹配错误"<<Qt::endl;
    return nullptr; // 如果没有匹配的精灵，返回空指针
}

pet::pet(QString name, int level, int exp, int attack, int defense, int hp, double interval, grade evolution)
{
    m_name = name;
    m_level = level;
    m_experience = exp;
    m_attack = attack;
    m_defense = defense;
    m_hp = hp;
    fight_hp = m_hp;
    m_attackInterval = interval;
    m_evolution = evolution;
}

QString pet::getName() const//获取名字
{
    return m_name;
}
int pet::getID() const//获取小精灵ID
{
    return m_ID;
}
int pet::getLevel() const//获取等级
{
    return m_level;
}
int pet::getExperience() const//获取经验
{
    return m_experience;
}
int pet::getAttack() const//获取攻击值
{
    return m_attack;
}
int pet::getDefense() const//获取防御值
{
    return m_defense;
}
int pet::getHP() const//获取血量
{
    return m_hp;
}
int pet::getfight_HP() const
{
    return fight_hp;
}
int pet::getfight_Attack() const
{
    return fight_attack;
}
int pet::getfight_Defense() const
{
    return fight_defense;
}
double pet::getfight_Interval() const
{
    return fight_attackInterval;
}
double pet::getAttackInterval() const//获取攻击间隔
{
    return m_attackInterval;
}
QString pet::getEvolution() const
{
    QString evolution;
    if(m_evolution == Normal)
        evolution = "Normal";
    else if(m_evolution == Evolution)
        evolution = "Evolution";
    else
        evolution = "SuperEvolution";
    return evolution;
}

void Attack::levelUp()
{
    if (m_level <= 15) {
        m_attack += 5;
        m_defense += 1;
        m_hp += 20;
        m_attackInterval -= 0.1;
        checkEvolution();
    }
}

void Defend::levelUp()
{
    if (m_level <= 15) {
        m_attack += 1;
        m_defense += 5;
        m_hp += 20;
        m_attackInterval -= 0.1;
        checkEvolution();
    }
}

void Hp::levelUp()
{
    if (m_level <= 15) {
        m_attack += 2;
        m_defense += 1;
        m_hp += 40;
        m_attackInterval -= 0.1;
        checkEvolution();
    }
}

void Speed::levelUp()
{
    if (m_level <= 15) {
        m_attack += 3;
        m_defense += 1;
        m_hp += 20;
        m_attackInterval -= 0.15;
        checkEvolution();
    }
}

void FireMonkey::attack(pet * enemy ,bool crt)
{
    int _HP;
    _HP = fight_attack;
    _HP = _HP*10/enemy->getfight_Defense(); //计算敌方小精灵hp减少值
    _HP = _HP * 1.5;//附加0.5倍的额外伤害
    QString logMessage = QString("%1火球，附加爆炸伤害").arg(this->getName());
    emit log(logMessage); // 发射 logMessage 信号
    if(crt)
        _HP = _HP*1.5;
    logMessage = QString("%1 攻击 %2，造成 %3 点伤害").arg(this->getName()).arg(enemy->getName()).arg(_HP);
    emit log(logMessage); // 发射 logMessage 信号
    enemy->setfight_HP(enemy->getfight_HP() - _HP);

}//炎火猴的攻击函数

void Cactus::attack(pet *enemy, bool crt)
{
    int _HP;
    _HP = fight_attack;
    QString logMessage = QString("%1穿刺，降低对方防御").arg(this->getName());
    emit log(logMessage); // 发射 logMessage 信号
    if(enemy->getfight_Defense() > 3)
    {
        int defense_2 = enemy->getfight_Defense();
        enemy->setfight_Defense(defense_2 - 2);
    }//每次受击减2防御
    _HP = _HP*10/enemy->getfight_Defense(); //计算敌方小精灵hp减少值
    if(crt)
        _HP = _HP*1.5;
    logMessage = QString("%1 攻击 %2，造成 %3 点伤害").arg(this->getName()).arg(enemy->getName()).arg(_HP);
    emit log(logMessage); // 发射 logMessage 信号
    enemy->setfight_HP(enemy->getfight_HP() - _HP);
}//仙人球的攻击函数

void tortoise::attack(pet *enemy,bool crt)
{
    int _HP;
    _HP = fight_attack;
    QString logMessage = QString("%1龟壳防御，增加自身防御力").arg(this->getName());
    emit log(logMessage); // 发射 logMessage 信号
    fight_defense = fight_defense + 1;
    _HP = _HP*10/enemy->getfight_Defense(); //计算敌方小精灵hp减少值
    if(crt)
        _HP = _HP*1.5;
    logMessage = QString("%1 攻击 %2，造成 %3 点伤害").arg(this->getName()).arg(enemy->getName()).arg(_HP);
    emit log(logMessage); // 发射 logMessage 信号
    enemy->setfight_HP(enemy->getfight_HP() - _HP);
}//贝尔的攻击函数

void shell::attack(pet *enemy,bool crt)
{
    int _HP;
    _HP = fight_attack;
    QString logMessage = QString("%1火焰防御，降低对方攻击").arg(this->getName());
    emit log(logMessage); // 发射 logMessage 信号
    if(enemy->getfight_Attack() > enemy->getLevel() + 5)
        enemy->setfight_Attack(enemy->getfight_Attack() - 1);
    _HP = _HP*10/enemy->getfight_Defense(); //计算敌方小精灵hp减少值
    if(crt)
        _HP = _HP*1.5;
    logMessage = QString("%1 攻击 %2，造成 %3 点伤害").arg(this->getName()).arg(enemy->getName()).arg(_HP);
    emit log(logMessage); // 发射 logMessage 信号
    enemy->setfight_HP(enemy->getfight_HP() - _HP);
}//火炎贝攻击函数

void seed::attack(pet *enemy,bool crt)
{
    int _HP;
    _HP = fight_attack;
    QString logMessage = QString("%1光合作用，回复生命").arg(this->getName());
    emit log(logMessage); // 发射 logMessage 信号
    if(this->getfight_HP() + 20 < this->getHP())
        this->setfight_HP(this->getfight_HP() + 20);
    else
        this->setfight_HP(this->getHP());
    _HP = _HP*10/enemy->getfight_Defense(); //计算敌方小精灵hp减少值
    if(crt)
        _HP = _HP*1.5;
    logMessage = QString("%1 攻击 %2，造成 %3 点伤害").arg(this->getName()).arg(enemy->getName()).arg(_HP);
    emit log(logMessage); // 发射 logMessage 信号
    enemy->setfight_HP(enemy->getfight_HP() - _HP);
}//布布种子攻击函数

void undine::attack(pet *enemy,bool crt)
{
    int _HP;
    _HP = fight_attack;
    QString logMessage = QString("%1沸腾，反弹部分已扣除血量").arg(this->getName());
    emit log(logMessage); // 发射 logMessage 信号
    _HP = _HP*10/enemy->getfight_Defense(); //计算敌方小精灵hp减少值
    _HP = _HP + (this->getHP() - this->getfight_HP()) * 0.15;
    if(crt)
        _HP = _HP*1.5;
    logMessage = QString("%1 攻击 %2，造成 %3 点伤害").arg(this->getName()).arg(enemy->getName()).arg(_HP);
    emit log(logMessage); // 发射 logMessage 信号
    enemy->setfight_HP(enemy->getfight_HP() - _HP);
}//伊修攻击函数

void mouse::attack(pet *enemy,bool crt)
{
    int _HP;
    _HP = fight_attack;
    QString logMessage = QString("%1闪电攻击，麻痹，每次攻击降低对方攻击速度").arg(this->getName());
    emit log(logMessage); // 发射 logMessage 信号
    enemy->setfight_Interval(enemy->getfight_Interval() + 0.1);
    _HP = _HP*10/enemy->getfight_Defense(); //计算敌方小精灵hp减少值
    if(crt)
        _HP = _HP*1.5;
    logMessage = QString("%1 攻击 %2，造成 %3 点伤害").arg(this->getName()).arg(enemy->getName()).arg(_HP);
    emit log(logMessage); // 发射 logMessage 信号
    enemy->setfight_HP(enemy->getfight_HP() - _HP);
}//比比鼠

void bat::attack(pet *enemy,bool crt)
{
    int _HP;
    _HP = fight_attack;
    QString logMessage = QString("%1超声波探测，每回合攻击间隔减少0.05").arg(this->getName());
    emit log(logMessage); // 发射 logMessage 信号
    if(this->getAttackInterval() > 0.2){
        this->setfight_Interval(this->getAttackInterval() - 0.05);
    }
    _HP = _HP*10/enemy->getfight_Defense(); //计算敌方小精灵hp减少值
    if(crt)
        _HP = _HP*1.5;
    logMessage = QString("%1 攻击 %2，造成 %3 点伤害").arg(this->getName()).arg(enemy->getName()).arg(_HP);
    emit log(logMessage); // 发射 logMessage 信号
    enemy->setfight_HP(enemy->getfight_HP() - _HP);
}
bool pet::isDead() const
{
    if(fight_hp <= 0)
        return true;
    else
        return false;
}//判断是否死亡
void pet::checkEvolution()
{
    if (m_level == 5) {
        if (m_evolution == Normal)
        {
            m_evolution = Evolution;
            m_attack = m_attack * 1.2;
            m_defense = m_defense * 1.2;
            m_hp = m_hp * 1.5;
            m_attackInterval -= 0.1;
        }
    }

    else if(m_level == 10){
        if (m_evolution == Evolution)
        {
            m_evolution = SuperEvolution;
            m_attack = m_attack * 1.2;
            m_defense = m_defense * 1.2;
            m_hp = m_hp * 1.5;
            m_attackInterval -= 0.1;
        }
    }

}//判断是否进化

void pet::setExp(int value)
{
    m_experience = value;
}//设置小精灵经验值
void pet::setAtk(int value)
{
    m_attack = value;
}//设置小精灵攻击值
void pet::setDefense(int value)
{
    m_defense = value;
}//设置小精灵防御值
void pet::setHp(int value)
{
    m_hp = value;
}//设置小精灵HP
void pet::setInterval(double value)
{
    m_attackInterval = value;
}//设置小精灵攻击间隔
void pet::setlevel(int value)
{
    m_level = value;
}//设置小精灵等级
void pet::setfight_HP(int value)
{
    fight_hp = value;
}//设置战斗时血量
void pet::setfight_Attack(int value)
{
    fight_attack = value;
}
void pet::setfight_Defense(int value)
{
    fight_defense = value;
}
void pet::setfight_Interval(double value)
{
    fight_attackInterval = value;
}
void pet::setID(int value)
{
    m_ID = value;
}
