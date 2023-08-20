#ifndef USERMODEL_H
#define USERMODEL_H

#include"user.hpp"

// 业务层只需要访问UserModel即可，该类方法里面全是对象不涉及对SQL语句和方法的调用

// User表的数据操作类
class UserModel{
public:
    // User表的增加方法
    bool insert(User& user);
    // 根据用户id查询用户信息
    User query(int id);
    // 更新用户状态信息
    bool updateState(User& user);

    // 重置用户状态信息
    void resetState();
};
#endif  // USERMODEL_H