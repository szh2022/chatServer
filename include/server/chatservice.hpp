#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>

#include <functional>
#include <mutex>
#include <unordered_map>
using namespace std;
using namespace muduo;
using namespace muduo::net;

#include "friendModel.hpp"
#include "groupmodel.hpp"
#include "json.hpp"
#include "offlinemessagemodel.hpp"
#include "usermodel.hpp"
using json = nlohmann::json;

// 处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp time)>;

// 业务模块，故有一个实例就够了用单例模式
// 聊天服务器业务类  给mesid映射一个事件回调
class ChatService {
public:
    static ChatService *instance();
    // login
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // registor
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 群组相关业务
    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    // 处理注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    // 服务器异常终止
    void reset();

private:
    ChatService();
    // 存储消息id及其对应的业务处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap;

    // 存储在线用户的通信连接（因为A和B聊天，A发给服务器，服务器要推给B，这时候服务器需要保存客户端的连接才行）
    unordered_map<int, TcpConnectionPtr> _userConnMap;  // 连接会随着用户的上线下线改变，需要注意线程安全

    // 数据操作类对象
    UserModel _userModel;
    // 离线消息操作类对象
    OfflineMsgModel _offlineMsgModel;
    // 好友类对象
    FriendModel _friendModel;
    // 群组相关类对象
    GroupModel _groupModel;

    // 定义互斥锁，保证_userConnMap的线程安全（多个用户并发登录，而STL里的insert不是线程安全的）
    mutex _connMutex;
};
#endif  // CHATSERVICE_H