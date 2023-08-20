#include <chrono>
#include <ctime>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "json.hpp"
using namespace std;
using json = nlohmann::json;

// 采用原始网络编程（因为不涉及高并发）
#include <arpa/inet.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>

#include "group.hpp"
#include "public.hpp"
#include "user.hpp"

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;

// 控制主菜单页面程序
bool isMainMenuRunning = false;

// 用于读写线程之间的通信
sem_t rwsem;

// 记录登录状态
atomic_bool g_isLoginSuccess{false};

// 显示当前登录用户的基本信息
void showCurrentUserData();

// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间（聊天时间需要添加时间信息）
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int clientfd);

// 显示当前登录用户的基本信息
void showCurrentUserData();

int main(int argc, char** argv) {
    if (argc < 3) {
        cerr << "command invalid! Example : ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }
    char* ip = argv[1];
    uint16_t port = atoi(argv[2]);
    int clientfd = socket(PF_INET, SOCK_STREAM, 0);
    if (clientfd == -1) {
        cerr << "socket create error!" << endl;
        exit(-1);
    }
    sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ip);

    int rt = connect(clientfd, (sockaddr*)&serv_addr, sizeof(serv_addr));
    if (rt < 0) {
        cerr << "connect server error" << endl;
        exit(-1);
    }

    // 初始化读写线程通信用的信号量
    sem_init(&rwsem, 0, 0);

    // 连接服务器成功，启动接收子线程
    std::thread readTask(readTaskHandler, clientfd);
    readTask.detach();

    // main线程用于接收用户输入，并发送数据
    while (true) {
        // 显示首页面菜单 登录、注册、退出
        cout << "========================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "========================" << endl;
        cout << "choice:";
        int choice = 0;
        cin >> choice;
        cin.get();  // 读掉缓冲区残留的回车
        switch (choice) {
        case 1:  // login
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "userid:";
            cin >> id;
            cin.get();
            cout << "user password:";
            cin.getline(pwd, 50);
            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            g_isLoginSuccess = false;
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len < 0)
                cerr << "send login msg error" << request << endl;

            sem_wait(&rwsem);  // 等待信号量，由子线程处理完登录的响应消息后，通知这里

            if (g_isLoginSuccess) {
                isMainMenuRunning = true;
                mainMenu(clientfd);
            }
        } break;
        case 2:  // register
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username:";
            cin.getline(name, 50);
            cout << "user password:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
                cerr << "send res msg error: " << request << endl;
            sem_wait(&rwsem);
        } break;
        case 3:  // quit
            close(clientfd);
            sem_destroy(&rwsem);
            exit(0);
            break;
        default:
            cerr << "invalid input!" << endl;
            break;
        }
    }

    return 0;
}

// 处理注册的响应逻辑
void doRegResponse(json& responsejs) {
    if (responsejs["errno"].get<int>() != 0)  // res fail
    {
        cerr << "name is already exist, register error!" << endl;
    } else {
        cout << "name register success, userid is" << responsejs["id"]
             << ", do not forget it!" << endl;
    }
}

// 处理登录的响应逻辑
void doLoginResponse(json& responsejs) {
    if (responsejs["errno"].get<int>() != 0) {  // login fail
        cerr << responsejs["errmsg"] << endl;
        g_isLoginSuccess = false;
    } else {
        // record id and name of curr user
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);

        // record the list of friend of user
        if (responsejs.contains("friends")) {
            // init
            g_currentUserFriendList.clear();
            vector<string> vec = responsejs["friends"];
            for (string& str : vec) {
                json js = json::parse(str);
                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }
        // record the group list of curr user
        if (responsejs.contains("groups")) {
            // init
            g_currentUserGroupList.clear();

            vector<string> vec1 = responsejs["groups"];
            for (string& groupstr : vec1) {
                json grpjs = json::parse(groupstr);
                Group group;
                group.setId(grpjs["id"].get<int>());
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);

                vector<string> vec2 = grpjs["users"];
                for (string& userstr : vec2) {
                    GroupUser user;
                    json js = json::parse(userstr);
                    user.setId(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);
                    group.getUsers().push_back(user);
                }
                g_currentUserGroupList.push_back(group);
            }
        }
        // show the info of curr user
        showCurrentUserData();

        // show offlinemesssage of curr user (one chat and group chat)
        if (responsejs.contains("offlinemsg")) {
            vector<string> vec3 = responsejs["offlinemsg"];
            for (string& str : vec3) {
                json js = json::parse(str);
                // time + [id] + name + "said: " + xxxx
                if (js["msgid"].get<int>() == ONE_CHAT_MSG) {
                    cout << js["time"].get<string>() << "[" << js["id"] << "]" << js["name"].get<string>()
                         << " said: " << js["msg"].get<string>() << endl;
                } else {
                    cout << "group message[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>()
                         << " said: " << js["msg"].get<string>() << endl;
                }
            }
        }
        g_isLoginSuccess = true;
    }
}

// 子线程 - 接收线程
void readTaskHandler(int clientfd) {
    while (true) {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);  // blocking
        if (len == -1 || len == 0) {
            close(clientfd);
            exit(-1);
        }
        // recv the data of chatserver ,and parse to json object
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (msgtype == ONE_CHAT_MSG) {
            cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        if (msgtype == GROUP_CHAT_MSG) {
            cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        if (msgtype == LOGIN_MSG_ACK) {
            doLoginResponse(js);  // 处理登录响应的业务逻辑
            sem_post(&rwsem);     // 通知主线程，登录结果处理完成
            continue;
        }
        if (msgtype == REG_MSG_ACK) {
            doRegResponse(js);
            sem_post(&rwsem);  // 通知主线程，注册结果处理完成
            continue;
        }
    }
}

// 显示当前登录成功用户的基本信息
void showCurrentUserData() {
    cout << "======================login user======================" << endl;
    cout << "current login user => id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << endl;
    cout << "----------------------friend list---------------------" << endl;
    if (!g_currentUserFriendList.empty()) {
        for (User& user : g_currentUserFriendList) {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "----------------------group list----------------------" << endl;
    if (!g_currentUserGroupList.empty()) {
        for (Group& group : g_currentUserGroupList) {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser& user : group.getUsers()) {
                cout << user.getId() << " " << user.getName() << " " << user.getState()
                     << " " << user.getRole() << endl;
            }
        }
    }
    cout << "======================================================" << endl;
}

// "help" command handler
void help(int fd = 0, string str = "");

// "chat" command handler
void chat(int fd, string str);

// "addfriend" command handler
void addfriend(int fd, string str);

// "creat group" command handler
void creategroup(int fd, string str);

// "add group" command handler
void addgroup(int fd, string str);

// "group chat" command handler
void groupchat(int fd, string str);

// "loginout" command handler
void loginout(int fd, string str);

// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群聊，格式groupchat:groupid:message"},
    {"loginout", "注销，格式loginout"}};
// 注册系统支持的客户端命令处理
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};

void mainMenu(int clientfd) {
    help();
    char buffer[1024] = {0};
    while (isMainMenuRunning) {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command;  // 存储命令
        int idx = commandbuf.find(":");
        if (idx == -1)
            command = commandbuf;
        else
            command = commandbuf.substr(0, idx);
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end()) {
            cerr << "invalid input command:" << command << endl;
            continue;
        }
        // 调用相应命令的事件处理回调，mainMenu对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - 1 - idx));  // 调用命令处理方法
    }
}

// "help" command handler
void help(int fd, string str) {
    cout << "show command list >>> " << endl;  // help
    for (auto& p : commandMap) {
        cout << p.first << " : " << p.second << endl;
    }
    cout << endl;
}

// "chat" command handler
void chat(int fd, string str) {
    int idx = str.find(":");  // friendid:message
    if (idx == -1) {
        cerr << "chat command invalid" << endl;
        return;
    }
    int friendid = atoi(str.substr(0, idx).c_str());
    string messages = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["to"] = friendid;
    js["msg"] = messages;
    js["time"] = getCurrentTime();
    string buffer = js.dump();
    int len = send(fd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
        cerr << "send chat msg error" << endl;
}

// "addfriend" command handler
void addfriend(int fd, string str) {
    int friendid = atoi(str.c_str());  // addfriend:friendid
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(fd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
        cerr << "send add friend msg error!" << endl;
}

// "creat group" command handler
void creategroup(int fd, string str) {
    int idx = str.find(":");  // creategroup:groupname:groupdesc
    if (idx == -1) {
        cerr << "creategroup command invalid!" << endl;
        return;
    }
    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();
    int len = send(fd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
        cerr << "send creategroup msg error!" << endl;
}

// "add group" command handler
void addgroup(int fd, string str) {
    int groupid = atoi(str.c_str());  // addgroup:groupid
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();
    int len = send(fd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
        cerr << "send addgroup msg error!" << endl;
}

// "group chat" command handler
void groupchat(int fd, string str) {
    int idx = str.find(":");  //  groupid:message
    if (idx == -1) {
        cerr << "groupchat command invalid!" << endl;
        return;
    }
    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();
    int len = send(fd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
        cerr << "send groupchat msg error!" << endl;
}

// "loginout" command handler
void loginout(int fd, string str) {  // loginout
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();
    int len = send(fd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
        cerr << "send loginout msg error!" << endl;
    else
        isMainMenuRunning = false;
}

// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime() {
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm* ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}
