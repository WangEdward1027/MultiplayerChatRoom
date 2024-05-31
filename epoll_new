#include <func.h>
#define MAX_CONNECT 3

//服务器端:使用epoll

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
    /* struct epoll_event* pevents = */
    /*             (struct epoll_event*)calloc(MAX_CONNECT, sizeof(struct epoll_event)); */
    
    int conns[MAX_CONNECT] = {0};

    //对IO事件进行循环监听
    while(1){
        int n = epoll_wait(epfd, events, MAX_CONNECT, -1); //n为已就绪的事件数

        if(n == -1 && errno == EINTR){
            continue;
        }else if(n == -1){
            perror("epoll_wait");
            break;
        }else if(n == 0){
            printf("timeout.\n");
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
                    if(conns[i] == 0){
                        conns[i] = peerfd;
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
                        close(fd); //关闭peerfd
                        printf("peerfd = %d has left.\n",fd);
                        continue;
                    }else
                    {
                        printf("ret:%d bytes, msg:%s",ret, buff);
                        //服务器群发消息给其他客户端
                        for(int j = 0; j < MAX_CONNECT; j++){
                            if(conns[j] > 0 && conns[j] != fd){ //不发送给自己
                                send(conns[j], buff, ret, 0);
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
