#include "chatservice.hpp"

#include <muduo/base/Logging.h>

#include <string>

#include "public.hpp"

using namespace std;
using namespace muduo;

// 获取单例对象的接口函数
ChatService *ChatService::instance() {
    static ChatService service;
    return &service;
}

// 构造方法
ChatService::ChatService() {
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
}

void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    LOG_INFO << "do login service!!!";
}
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    LOG_INFO << "do reg service!!!";
}

MsgHandler ChatService::getHandler(int msgid) {
    // 记录错误日志，msgid 没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end()) {
        // 返回一个默认处理器，空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time) {
            LOG_ERROR << "msgig:" << msgid << " can not find handler!";
        };
    } else {
        return _msgHandlerMap[msgid];
    }
}