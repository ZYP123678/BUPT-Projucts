#ifndef PET_H
#define PET_H
#include<Qstring.h>

class pet
{


public:
    enum grade {
        Normal,
        Evolution,
        SuperEvolution
    };
    pet();
    pet(QString name, int level, int exp, int attack, int defense, int hp, double interval, grade evolution);

    virtual void levelUp() = 0;//升级
    virtual void attack(pet *) = 0;//攻击函数
    void attacked(int value) ;//被攻击函数

    //属性
    QString m_name;
    int m_ID;
    int m_level;
    int m_experience;
    int m_attack;
    int m_defense;
    int m_hp;
    double m_attackInterval;
    grade m_evolution;


};
//攻击型
class Attack:public pet{
    virtual void attack(pet *) = 0;//攻击函数
    void levelUp();//升级函数
};
//炎火猴
class FireMonkey:public Attack{
    FireMonkey(int _level, grade _evolution, int _exp, int _attack, int _defense, int _hp, int _interval) {
        m_name = "炎火猴";
        m_level = _level;
        m_experience = _exp;
        m_evolution = _evolution;
        m_attack = _attack;
        m_defense = _defense;
        m_hp = _hp;
        m_attackInterval = _interval;
    }

    FireMonkey() {
        m_name = "炎火猴";
    }
    void attack(pet *enemy);
    ~FireMonkey() {}
};
//仙人球
class Cactus:public Attack{
    Cactus(int _level, grade _evolution, int _exp, int _attack, int _defense, int _hp, int _interval) {
        m_name = "仙人球";
        m_level = _level;
        m_experience = _exp;
        m_evolution = _evolution;
        m_attack = _attack;
        m_defense = _defense;
        m_hp = _hp;
        m_attackInterval = _interval;
    }

    Cactus() {
        m_name = "仙人球";
    }
    void attack(pet *enemy);
    ~Cactus() {}
};
//肉盾型
class Defend:public pet{
    virtual void attack(pet *) = 0;//攻击函数
    void levelUp();//升级函数
};
//贝尔
class tortoise:public Defend{
    tortoise(int _level, grade _evolution, int _exp, int _attack, int _defense, int _hp, int _interval) {
        m_name = "贝尔";
        m_level = _level;
        m_experience = _exp;
        m_evolution = _evolution;
        m_attack = _attack;
        m_defense = _defense;
        m_hp = _hp;
        m_attackInterval = _interval;
    }

    tortoise() {
        m_name = "贝尔";
    }
    void attack(pet *enemy);
    ~tortoise() {}
};
//火炎贝
class shell:public Defend{
    shell(int _level, grade _evolution, int _exp, int _attack, int _defense, int _hp, int _interval) {
        m_name = "火炎贝";
        m_level = _level;
        m_experience = _exp;
        m_evolution = _evolution;
        m_attack = _attack;
        m_defense = _defense;
        m_hp = _hp;
        m_attackInterval = _interval;
    }

    shell() {
        m_name = "火炎贝";
    }
    void attack(pet *enemy);
    ~shell() {}
};
//肉盾型
class Hp:public pet{
    virtual void attack(pet *) = 0;//攻击函数
    void levelUp();//升级函数
};
//布布种子
class seed:public Hp{
    seed(int _level, grade _evolution, int _exp, int _attack, int _defense, int _hp, int _interval) {
        m_name = "布布种子";
        m_level = _level;
        m_experience = _exp;
        m_evolution = _evolution;
        m_attack = _attack;
        m_defense = _defense;
        m_hp = _hp;
        m_attackInterval = _interval;
    }

    seed() {
        m_name = "布布种子";
    }
    void attack(pet *enemy);
    ~seed() {}
};
//伊修
class undine : public Hp{
    undine(int _level, grade _evolution, int _exp, int _attack, int _defense, int _hp, int _interval) {
        m_name = "伊修";
        m_level = _level;
        m_experience = _exp;
        m_evolution = _evolution;
        m_attack = _attack;
        m_defense = _defense;
        m_hp = _hp;
        m_attackInterval = _interval;
    }

    undine() {
        m_name = "伊修";
    }
    void attack(pet *enemy);
    ~undine() {}
};
//敏捷型
class Speed:public pet{
    virtual void attack(pet *) = 0;//攻击函数
    void levelUp();//升级函数
};
//比比鼠
class mouse:public Speed{
    mouse(int _level, grade _evolution, int _exp, int _attack, int _defense, int _hp, int _interval) {
        m_name = "比比鼠";
        m_level = _level;
        m_experience = _exp;
        m_evolution = _evolution;
        m_attack = _attack;
        m_defense = _defense;
        m_hp = _hp;
        m_attackInterval = _interval;
    }

    mouse() {
        m_name = "比比鼠";
    }
    void attack(pet *enemy);
    ~mouse() {}
};
//悠悠
class bat:public Speed{
    bat(int _level, grade _evolution, int _exp, int _attack, int _defense, int _hp, int _interval) {
        m_name = "悠悠";
        m_level = _level;
        m_experience = _exp;
        m_evolution = _evolution;
        m_attack = _attack;
        m_defense = _defense;
        m_hp = _hp;
        m_attackInterval = _interval;
    }

    bat() {
        m_name = "悠悠";
    }
    void attack(pet *enemy);
    ~bat() {}
};

#endif // PET_H
