#ifndef GROUP_USER_H
#define GROUP_USER_H
#include"user.hpp"
// 群组成员，多了一个role角色信息，从User类继承，复用User其他信息
class GroupUser:public User{
public:
    void setRole(string role){this->role = role;}
    string getRole(){return this->role;}
private:
    string role;
};
#endif //GROUP_USER_H