#ifndef MANAGER_H
#define MANAGER_H
#include <define_settings.h>
#include <qstring.h>

class Manager
{
public:
    explicit Manager();
    int socketID;
    QString username;//玩家姓名
    int userID;//玩家ID
    int pet_num;
    int money;
    int drug;
};

#endif // MANAGER_H
