#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>

#include <functional>
#include <unordered_map>
using namespace std;
using namespace muduo;
using namespace muduo::net;

#include "json.hpp"
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
    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);

private:
    ChatService();
    // 存储消息id及其对应的业务处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap;
};
#endif  // CHATSERVICE_H