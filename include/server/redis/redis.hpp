#ifndef REDIS_H
#define REDIS_H

#include<hiredis/hiredis.h>
#include<thread>
#include<functional>

using namespace std;
/*
redis作为集群服务器通信的基于发布-订阅消息队列时，会遇到两个难搞的bug问题，参考博客详细描述：
https://blog.csdn.net/QIANGWEIYUAN/article/details/97895611
*/
class Redis{
public:
    Redis();
    ~Redis();

    // connect to redis
    bool connect();
    // publish to redis
    bool publish(int channel,string message);
    // subscribe channel from redis
    bool subscribe(int channnel);
    // unsubscribe channel
    bool unsubscribe(int channel);
    // recv message from subscribe channel in dependent thread
    void observer_channel_message();
    // 初始化向业务层上报通道消息的回调对象
    void init_notify_handler(function<void(int,string)> fn);
private:
    // hiredis同步上下文对象，负责publish消息   
    redisContext * _publish_context;
    // hiredis同步上下文对象，负责subscribe消息
    redisContext * _subscribe_context;
    // 回调操作，收到订阅的消息，给service层上报
    function<void(int,string)> _notify_message_handler;
};
#endif // REDIS_H