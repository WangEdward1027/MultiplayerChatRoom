#include <func.h>
#define MAX_CONNECT 3
#define TIMEOUT 10

//服务器端:使用epoll
//带有超时踢出功能，20秒不发言，则服务器关闭该客户端的连接
//结构体保存上次发言时间

typedef struct conn_s {
    int peerfd;         //保存与对端进行交互的peerfd
    int lastActiveTime; //保存该连接上一次通信的时间(秒)
} conn_info_t;

int main(void)
{
    //创建监听套接字
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd == -1){
        error(1, errno, "socket");
    }

    //填充网络地址
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(8080);
    serveraddr.sin_addr.s_addr = inet_addr("192.168.248.136");

    //设置套接字属性，网络地址可重用
    int on = 1; //1表示有效
    int ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
    if(ret == -1){
        error(1, errno, "setsockopt");
    }

    //绑定网络地址
    ret = bind(listenfd, (const struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if(ret == -1){
        error(1, errno, "bind");
    }

    //进行监听
    ret = listen(listenfd, 10);
    if(ret == -1){
        error(1, errno, "listen");
    }
    printf("server start listening.\n");

    //创建epoll的实例
    int epfd = epoll_create1(0);
    if(epfd == -1){
        error(1, errno, "epoll_create1");
    }

    //将监听的socket加入epoll (对响应的文件描述符进行监听)
    struct epoll_event ev;
    ev.events = EPOLLIN;    //套接字可读, 关注listenfd的读事件
    ev.data.fd = listenfd;

    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev); //注册listenfd
    if(ret == -1){
        error(1, errno, "epoll_ctl");
    }

    struct epoll_event events[MAX_CONNECT];
    conn_info_t conns[MAX_CONNECT] = {0};
    
    /* int conns[MAX_CONNECT] = {0}; */
    /* time_t last_active[MAX_CONNECT] = {0}; //记录每个客户端最后活跃的时间 */

    //对IO事件进行循环监听
    while(1){
        int n = epoll_wait(epfd, events, MAX_CONNECT, 1000); //每隔一秒,判断一次超时

        if(n == -1 && errno == EINTR){
            continue;
        }else if(n == -1){
            perror("epoll_wait");
            break;
        }else if(n == 0){
            //超时处理
            time_t curTime = time(NULL);
            for(int i = 0; i < MAX_CONNECT; i++){
                if(conns[i].peerfd > 0 && (curTime - conns[i].lastActiveTime >= TIMEOUT)){
                    //发送超时通知给客户端
                    printf("peerfd = %d timed out.\n", conns[i].peerfd);
                    const char *timeout_msg = "超时未发言，您已被踢出聊天室。\n";
                    send(conns[i].peerfd, timeout_msg, strlen(timeout_msg), 0);

                    //从监听的红黑树删除
                    ev.data.fd = conns[i].peerfd;
                    ret = epoll_ctl(epfd, EPOLL_CTL_DEL, conns[i].peerfd, &ev);
                    if(ret == -1){
                        error(1, errno, "epoll_ctl");
                    }

                    close(conns[i].peerfd); //关闭超时的peerfd
                    //清空peerfd的记录
                    conns[i].peerfd = 0;    //标记为可用
                    conns[i].lastActiveTime = 0;
                }
            }
            continue;
        }
        //n > 0 :对 n个文件描述符进行处理
        for(int i = 0; i < n; i++){
            int fd = events[i].data.fd;
            //对listenfd的处理，建立新的连接
            if(fd == listenfd){
                //接收服务器的连接请求(阻塞)
                struct sockaddr_in client_addr;
                memset(&client_addr, 0, sizeof(client_addr));
                socklen_t len = sizeof(client_addr);

                //服务器接收客户端的connect连接请求
                int peerfd = accept(listenfd, (struct sockaddr*)&client_addr, &len);
                if(peerfd == -1){
                    perror("accept");
                    continue;
                }

                //将客户端的socket加入epoll
                printf("%s:%d has connected.\n",
                       inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
                
                struct epoll_event ev;
                ev.events = EPOLLIN;
                ev.data.fd = peerfd;
                ret = epoll_ctl(epfd, EPOLL_CTL_ADD, peerfd, &ev); //注册peerfd
                if(ret == -1){
                    error(1, errno, "epoll_ctl");
                }

                //存储已建立好的连接
                for(int i = 0; i < MAX_CONNECT; i++){
                    if(conns[i].peerfd == 0){
                        conns[i].peerfd = peerfd;
                        conns[i].lastActiveTime = time(NULL); //记录活跃时间
                        break;
                    }
                }
            }
            else
            { //fd == peerfd:处理peerfd
                if(events[i].events & EPOLLIN){ //判断是否发生了读事件
                    char buff[1024] = {0};

                    //recv返回值的三种情况
                    int ret = recv(fd, buff, sizeof(buff), 0);
                    if(ret == -1)
                    {
                        error(1, errno, "recv");
                    }
                    else if(ret == 0)
                    {
                        //连接断开的情况
                        ev.data.fd = fd;
                        //从监听的红黑树删除
                        ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
                        if(ret == -1){
                            error(1, errno, "epoll_ctl");
                        }
                        //更新conns中的信息
                        for(int i = 0; i < MAX_CONNECT; i++){
                            if(conns[i].peerfd == fd){
                                conns[i].peerfd = 0; //标记为可用
                                conns[i].lastActiveTime = 0;
                                break;
                            }
                        }
                        
                        close(fd); //关闭peerfd
                        printf("peerfd = %d has left.\n",fd);
                        continue;
                    }
                    else
                    {
                        printf("ret:%d bytes, msg:%s",ret, buff);
                        //更新活跃时间
                        for(int i = 0; i < MAX_CONNECT; i++){
                            if(conns[i].peerfd == fd){
                                conns[i].lastActiveTime = time(NULL);
                                break;
                            }
                        }
                        //服务器将消息转发给其他客户端
                        for(int j = 0; j < MAX_CONNECT; j++){
                            if(conns[j].peerfd > 0 && conns[j].peerfd != fd){ //不发送给自己
                                send(conns[j].peerfd, buff, ret, 0);
                            }
                        }
                    }
                }
            }
        }
    }
    close(listenfd);
    close(epfd);

    return 0;
}
