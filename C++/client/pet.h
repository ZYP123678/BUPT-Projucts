#ifndef PET_H
#define PET_H
#include<Qwidget>
#include <QObject>
#include "qdebug.h"
#include<QString>
#include <cstdlib>
#include <ctime>
#include <vector>



class pet : public QObject
{
    Q_OBJECT


public:
    enum grade {
        Normal,
        Evolution,
        SuperEvolution
    };
    pet();
    pet(QString name, int level, int exp, int attack, int defense, int hp, double interval, grade evolution);
    QString getName() const;//获取名字
    int getID() const;//获取小精灵ID
    int getLevel() const;//获取等级
    int getExperience() const;//获取经验
    int getAttack() const;//获取攻击值
    int getDefense() const;//获取防御值
    int getHP() const;//获取血量
    QString getEvolution() const;//获取进化等级
    double getAttackInterval() const;//获取攻击间隔
    int getfight_HP() const;//获取战斗时血量
    int getfight_Attack() const;//获取战斗时攻击
    int getfight_Defense() const;//获取战斗时防御
    double getfight_Interval() const;//获取战斗时攻击间隔

    virtual void levelUp() = 0;//升级
    virtual void attack(pet * enemy, bool crt) = 0;//攻击函数

    bool isDead() const;//判断是否死亡
    void checkEvolution();//判断是否进化

    void setExp(int value);//设置小精灵经验值
    void setAtk(int value);//设置小精灵攻击值
    void setDefense(int value);//设置小精灵防御值
    void setHp(int value);//设置小精灵HP
    void setInterval(double value);//设置小精灵攻击间隔
    void setlevel(int value);//设置小精灵等级
    void setfight_HP(int value);//设置战斗时血量
    void setfight_Attack(int value);//设置战斗时攻击
    void setfight_Defense(int value);//设置战斗时防御
    void setfight_Interval(double value);//设置战斗时攻击间隔
    void setID(int ID);//设置精灵ID

    static pet* createRandomPet(int level); // 静态方法用于生成随机精灵
    //grade parseEvolution(const QString &evolutionStr);//解析进化等级

signals:
    void log(QString &message);

protected:
    //属性
    QString m_name;
    int m_ID;
    int m_level;
    int m_experience;
    int m_attack;
    int m_defense;
    int m_hp;
    double m_attackInterval;

    QString attackname;
    grade m_evolution;

    int fight_hp;
    int fight_attack;
    int fight_defense;
    double fight_attackInterval;

private:
    static std::vector<QString> names;

};
//攻击型
class Attack:public pet{
    Q_OBJECT
public:
    virtual void attack(pet * enemy, bool crt) = 0;//攻击函数
    void levelUp();//升级函数
};
//炎火猴
class FireMonkey:public Attack{
    Q_OBJECT
public:
    FireMonkey(int _level, grade _evolution, int _exp, int _attack, int _defense, int _hp, double _interval) {
        m_name = "炎火猴";
        m_level = _level;
        fight_hp = _hp;
        fight_attack = _attack;
        fight_defense = _defense;
        fight_attackInterval = _interval;
        m_experience = _exp;
        m_evolution = _evolution;
        m_attack = _attack;
        m_defense = _defense;
        m_hp = _hp;
        m_attackInterval = _interval;
    }
    FireMonkey(int level) {
        m_name = "炎火猴";
        int i = 1;
        for(i = 1; i < level;i++){
            m_level++;
            levelUp();
        }
        fight_hp = m_hp;
        fight_hp = m_hp;
        fight_attack = m_attack;
        fight_attackInterval = m_attackInterval;
        fight_defense = m_defense;
    }

    FireMonkey() {
        m_name = "炎火猴";
    }
    void attack(pet *enemy , bool crt);
    ~FireMonkey() {}
};
//仙人球
class Cactus:public Attack{
    Q_OBJECT
public:
    Cactus(int _level, grade _evolution, int _exp, int _attack, int _defense, int _hp, double _interval) {
        m_name = "仙人球";
        m_level = _level;
        m_experience = _exp;
        m_evolution = _evolution;
        m_attack = _attack;
        m_defense = _defense;
        m_hp = _hp;
        fight_hp = _hp;
        m_attackInterval = _interval;
        fight_hp = _hp;
        fight_attack = _attack;
        fight_defense = _defense;
        fight_attackInterval = _interval;
    }
    Cactus(int level) {
        m_name = "仙人球";
        int i = 1;
        for(i = 1; i < level;i++){
            m_level++;
            levelUp();
        }
        fight_hp = m_hp;
        fight_hp = m_hp;
        fight_attack = m_attack;
        fight_attackInterval = m_attackInterval;
        fight_defense = m_defense;
    }
    Cactus() {
        m_name = "仙人球";
    }
    void attack(pet *enemy , bool crt);
    ~Cactus() {}
};
//肉盾型
class Defend:public pet{
    Q_OBJECT
public:
    virtual void attack(pet * enemy, bool crt) = 0;//攻击函数
    void levelUp();//升级函数
};
//贝尔
class tortoise:public Defend{
    Q_OBJECT
public:
    tortoise(int _level, grade _evolution, int _exp, int _attack, int _defense, int _hp, double _interval) {
        m_name = "贝尔";
        m_level = _level;
        m_experience = _exp;
        m_evolution = _evolution;
        m_attack = _attack;
        m_defense = _defense;
        m_hp = _hp;
        fight_hp = _hp;
        m_attackInterval = _interval;
        fight_hp = _hp;
        fight_attack = _attack;
        fight_defense = _defense;
        fight_attackInterval = _interval;
    }
    tortoise(int level) {
        m_name = "贝尔";
        int i = 1;
        for(i = 1; i < level;i++){
            m_level++;
            levelUp();
        }
        fight_hp = m_hp;
        fight_hp = m_hp;
        fight_attack = m_attack;
        fight_attackInterval = m_attackInterval;
        fight_defense = m_defense;
    }
    tortoise() {
        m_name = "贝尔";
    }
    void attack(pet *enemy , bool crt);
    ~tortoise() {}
};
//火炎贝
class shell:public Defend{
    Q_OBJECT
public:
    shell(int _level, grade _evolution, int _exp, int _attack, int _defense, int _hp, double _interval) {
        m_name = "火炎贝";
        m_level = _level;
        m_experience = _exp;
        m_evolution = _evolution;
        m_attack = _attack;
        m_defense = _defense;
        m_hp = _hp;
        fight_hp = _hp;
        m_attackInterval = _interval;
        fight_hp = _hp;
        fight_attack = _attack;
        fight_defense = _defense;
        fight_attackInterval = _interval;
    }
    shell(int level) {
        m_name = "火炎贝";
        int i = 1;
        for(i = 1; i < level;i++){
            m_level++;
            levelUp();
        }
        fight_hp = m_hp;
        fight_hp = m_hp;
        fight_attack = m_attack;
        fight_attackInterval = m_attackInterval;
        fight_defense = m_defense;
    }
    shell() {
        m_name = "火炎贝";
    }
    void attack(pet *enemy , bool crt);
    ~shell() {}
};
//肉盾型
class Hp:public pet{
    Q_OBJECT
public:
    virtual void attack(pet * enemy, bool crt) = 0;//攻击函数
    void levelUp();//升级函数
};
//布布种子
class seed:public Hp{
    Q_OBJECT
public:
    seed(int _level, grade _evolution, int _exp, int _attack, int _defense, int _hp, double _interval) {
        m_name = "布布种子";
        m_level = _level;
        m_experience = _exp;
        m_evolution = _evolution;
        m_attack = _attack;
        m_defense = _defense;
        m_hp = _hp;
        fight_hp = _hp;
        m_attackInterval = _interval;
        fight_hp = _hp;
        fight_attack = _attack;
        fight_defense = _defense;
        fight_attackInterval = _interval;
    }
    seed(int level) {
        m_name = "布布种子";
        int i = 1;
        for(i = 1; i < level;i++){
            m_level++;
            levelUp();
        }
        fight_hp = m_hp;
        fight_hp = m_hp;
        fight_attack = m_attack;
        fight_attackInterval = m_attackInterval;
        fight_defense = m_defense;
    }
    seed() {
        m_name = "布布种子";
    }
    void attack(pet *enemy , bool crt);
    ~seed() {}
};
//伊修
class undine : public Hp{
    Q_OBJECT
public:
    undine(int _level, grade _evolution, int _exp, int _attack, int _defense, int _hp, double _interval) {
        m_name = "伊修";
        m_level = _level;
        m_experience = _exp;
        m_evolution = _evolution;
        m_attack = _attack;
        m_defense = _defense;
        m_hp = _hp;
        fight_hp = _hp;
        m_attackInterval = _interval;
        fight_hp = _hp;
        fight_attack = _attack;
        fight_defense = _defense;
        fight_attackInterval = _interval;
    }
    undine(int level) {
        m_name = "伊修";
        int i = 1;
        for(i = 1; i < level;i++){
            m_level++;
            levelUp();
        }
        fight_hp = m_hp;
        fight_hp = m_hp;
        fight_attack = m_attack;
        fight_attackInterval = m_attackInterval;
        fight_defense = m_defense;
    }
    undine() {
        m_name = "伊修";
    }
    void attack(pet *enemy , bool crt);
    ~undine() {}
};
//敏捷型
class Speed:public pet{
    Q_OBJECT
public:
    virtual void attack(pet * enemy, bool crt) = 0;//攻击函数
    void levelUp();//升级函数
};
//比比鼠
class mouse:public Speed{
    Q_OBJECT
public:
    mouse(int _level, grade _evolution, int _exp, int _attack, int _defense, int _hp, double _interval) {
        m_name = "比比鼠";
        m_level = _level;
        m_experience = _exp;
        m_evolution = _evolution;
        m_attack = _attack;
        m_defense = _defense;
        m_hp = _hp;
        fight_hp = _hp;
        m_attackInterval = _interval;
        fight_hp = _hp;
        fight_attack = _attack;
        fight_defense = _defense;
        fight_attackInterval = _interval;
    }
        mouse(int level) {
        m_name = "比比鼠";
        int i = 1;
        for(i = 1; i < level;i++){
            m_level++;
            levelUp();
        }
        fight_hp = m_hp;
        fight_hp = m_hp;
        fight_attack = m_attack;
        fight_attackInterval = m_attackInterval;
        fight_defense = m_defense;
    }
    mouse() {
        m_name = "比比鼠";
    }
    void attack(pet *enemy , bool crt);
    ~mouse() {}
};
//悠悠
class bat:public Speed{
    Q_OBJECT
public:
    bat(int _level, grade _evolution, int _exp, int _attack, int _defense, int _hp, double _interval) {
        m_name = "悠悠";
        m_level = _level;
        m_experience = _exp;
        m_evolution = _evolution;
        m_attack = _attack;
        m_defense = _defense;
        m_hp = _hp;
        fight_hp = _hp;
        m_attackInterval = _interval;
        fight_hp = _hp;
        fight_attack = _attack;
        fight_defense = _defense;
        fight_attackInterval = _interval;
    }
    bat(int level) {
        m_name = "悠悠";
        int i = 1;
        for(i = 1; i < level;i++){
            m_level++;
            levelUp();
        }
        fight_hp = m_hp;
        fight_hp = m_hp;
        fight_attack = m_attack;
        fight_attackInterval = m_attackInterval;
        fight_defense = m_defense;
    }
    bat() {
        m_name = "悠悠";
    }
    void attack(pet *enemy , bool crt);
    ~bat() {}
};

#endif // PET_H
