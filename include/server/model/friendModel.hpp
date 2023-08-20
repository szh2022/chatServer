#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H
#include<vector>
#include"user.hpp"
using namespace std;

// 维护好友操作接口方法
class FriendModel{
public:
    // 添加好友关系（按理说是应该存在客户端的，但是这里为了方便每次客户端登录，服务器把好友信息发给客户端）
    void insert(int userid,int friendid);
    // 返回用户好友列表
    vector<User> query(int userid);
};

#endif // FRIENDMODEL_H