#include <func.h>

//服务器端:一对一聊天，基于TCP协议：socket、bind、listen、accept、recv、send、close
//使用select监听stdin和客户端

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
    
    //设置网络地址可重用(套接字属性)，服务器主动断开连接，无需等待2MSL时间便可重用IP地址
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
    
    //接收服务器的连接请求(阻塞)
    struct sockaddr_in clientaddr;
    memset(&clientaddr, 0, sizeof(clientaddr));
    socklen_t len = sizeof(clientaddr);
    //一对一聊天室,服务器仅接收一个客户端的connect连接请求
    int peerfd = accept(listenfd, (struct sockaddr*)&clientaddr, &len);
    printf("%s:%d has connected.\n",inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));

    fd_set readset;
    char buff[1024] = {0};

    //对IO事件进行监听
    while(1){
        FD_ZERO(&readset);
        FD_SET(peerfd, &readset);
        FD_SET(STDIN_FILENO, &readset);

        select(peerfd + 1, &readset, NULL, NULL, NULL); //只监听读事件

        if(FD_ISSET(STDIN_FILENO, &readset)){
            //清空缓冲区
            memset(buff, 0, sizeof(buff));
            //从标准输出获取数据
            ret = read(STDIN_FILENO, buff, sizeof(buff));
            if(ret == 0){
                break;
            }
            printf("read stdin %d bytes, ",ret);
            //发送给对端
            ret = send(peerfd, buff, strlen(buff), 0);
            printf("send %d bytes.\n",ret);
        }

        if(FD_ISSET(peerfd, &readset)){
            //清空缓冲区
            memset(buff, 0, sizeof(buff));
            //从对端获取数据
            ret = recv(peerfd, buff, sizeof(buff), 0);
            if(ret == 0){
                //连接已经断开
                break;
            }
            printf("recv msg: %s", buff);
        }
    }
    printf("对方已离开聊天室。\n");

    close(listenfd);
    close(peerfd);

    return 0;
}
