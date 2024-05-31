//服务器端
#include <cstdio>
#include <iostream>
#include <string>
#include <sys/epoll.h>  //epoll的头文件
#include <sys/socket.h> //socket的头文件
#include <unistd.h>     //close()的头文件
#include <netinet/in.h> //包含结构体 sockaddr_in
#include <map>          //保存客户端信息
#include <arpa/inet.h>  //提供inet_ntoa函数
using namespace std;

const int MAX_CONNECT = 5; //全局静态变量,允许的最大连接数

struct Client{
    int sockfd; //socket file descriptor 套接字文件描述符
    string username;
};

//1.处理新的客户端连接请求
void handle_new_connection(int epoll_fd, int server_fd, map<int,Client> &clients, int &clientCount){
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_sockfd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
    if(client_sockfd < 0) {
        perror("accept error");
        return;
    }
    
    //将客户端的socket加入epoll
    struct epoll_event ev_client;
    ev_client.events = EPOLLIN;  //检测客户端有没有消息过来
    ev_client.data.fd = client_sockfd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sockfd, &ev_client) < 0) {
        perror("epoll_ctl error");
        close(client_sockfd);
        return;
    }
    
    clientCount++;  //有新的客户端加入时,增加计数器
    
    //iner_ntoa() 将客户端的IP地址从网络字节顺序转换为点分十进制字符串
    printf("客户端%d已连接: IP地址为 %s\n", clientCount, inet_ntoa(client_addr.sin_addr));
    
    //保存该客户端信息
    Client client;
    client.sockfd = client_sockfd;
    clients[client_sockfd] = client;
}

//2.处理客户端消息
void handle_client_message(int epoll_fd, int client_fd, map<int,Client> &clients){
    char buffer[1024];
    int n = read(client_fd, buffer, sizeof(buffer));
    //1.read()<0:错误
    if(n < 0){ 
        perror("read error");
        return;
    }
    //2.read()==0:读取结束
	else if(n == 0){ 
        printf("客户端断开连接\n");
        close(client_fd);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
        clients.erase(client_fd);
        return;
    }
    //3.read()>0:处理消息
    string msg(buffer, n);
    if(clients[client_fd].username == "") {
        clients[client_fd].username = msg; //如果该客户端username为空,说明该消息是这个客户端的用户名
    }else{
        for(auto & c: clients) {
            string name = clients[client_fd].username;
            if(c.first != client_fd){  //不为自己
                write(c.first,('[' + name + ']' + ":" + msg).c_str(),msg.size()+name.size()+3);
            }
        }
    }
}


int main(){
    //创建一个epoll实例
    int epfd = epoll_create1(0); //或老版本 epoll_create(1);
    if(epfd < 0){
        perror("epoll create error");
        return -1;
    }

    //创建监听的socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){ //若socket创建失败,则返回-1
        perror("socket error");
        return -1;
    }

    //绑定本地ip和端口
    struct sockaddr_in addr;  //结构体声明,头文件是<netinet/in.h>
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port  = htons(9999);

    int ret = bind(sockfd,(struct sockaddr*)&addr,sizeof(addr));
    if(ret < 0){
        printf("bind error\n");
        cout << "该端口号已被占用,请检查服务器是否已经启动。" << endl;
        return -1;
    }

    cout << "服务器中转站已启动,请加入客户端。" << endl;

    //监听客户端
    ret = listen(sockfd,1024);
    if(ret < 0){
        printf("listen error\n");
        return -1;
    }

    //将监听的socket加入epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;  //套接字可读
    ev.data.fd = sockfd;

    ret = epoll_ctl(epfd,EPOLL_CTL_ADD,sockfd,&ev); //epoll_ctl():监控
    if(ret < 0){        //防御性编程,方便出bug时快速定位问题
        printf("epoll_ctl error\n");
        return -1;
    }

    //保存客户端信息
    map<int,Client> clients;
    int clientCount = 0; //添加一个客户端计数器

    //循环监听
    while(true){
        struct epoll_event evs[MAX_CONNECT];
        int n = epoll_wait(epfd,evs,MAX_CONNECT,-1);
        if(n < 0){
            printf("epoll_wait error\n");
            break;
        }

        for(int i = 0; i < n; i++) {
            if(evs[i].data.fd == sockfd){ //1.处理:有新的客户端连接请求
                handle_new_connection(epfd, sockfd, clients, clientCount);
            }else{                        //2.处理:已连接的客户端发送消息
                handle_client_message(epfd, evs[i].data.fd, clients);
            }
        }
    }
    //关闭epoll实例
    close(epfd);
    close(sockfd);
    return 0;
}
