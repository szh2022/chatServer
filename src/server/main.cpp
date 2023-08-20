#include <signal.h>

#include <iostream>

#include "chatserver.hpp"
#include"chatservice.hpp"
using namespace std;

// 处理ctrl + c结束后，重置user的状态信息
void resetHandler(int){
    ChatService::instance()->reset();
    exit(0);
}

// int main() {
//     signal(SIGINT,resetHandler);  // 暂时只处理ctrl+c结束后的状态重置
//     EventLoop loop;
//     InetAddress addr("127.0.0.1", 10000);
//     ChatServer server(&loop, addr, "ChatServer");
//     server.start();
//     loop.loop();
//     return 0;
// }
int main(int argc,char **argv){
    if(argc < 3){
        cerr<<"Command invalid! example :./ChatServer 127.0.0.1 6000"<<endl;
        exit(-1);
    }
    char * ip = argv[1];
    uint16_t port = atoi(argv[2]);
    signal(SIGINT, resetHandler);
    EventLoop loop;
    InetAddress addr(ip,port);
    ChatServer server(&loop,addr,"ChatServer");

    server.start();
    loop.loop();
    return 0;
}