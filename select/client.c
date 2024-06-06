#include <func.h>

//客户端

int main(void)
{
    //创建客户端套接字
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(clientfd == -1)    error(1, errno, "socket");

    //填充网络地址
    struct sockaddr_in clientaddr;
    memset(&clientaddr, 0, sizeof(clientaddr)); //初始化
    clientaddr.sin_family = AF_INET;
    clientaddr.sin_port = htons(8080);
    clientaddr.sin_addr.s_addr = inet_addr("192.168.248.136");
    
    //客户端发起连接请求:connect
    int ret = connect(clientfd,(const struct sockaddr *)&clientaddr, sizeof(clientaddr));
    if(ret == -1)    error(1, errno, "connect");
    printf("connect to server success\n");
 
    fd_set readset; //读集合
    char buff[1024] = {0};

    //对IO事件进行监听
    while(1){
        FD_ZERO(&readset);
        FD_SET(clientfd, &readset);
        FD_SET(STDIN_FILENO,&readset);

        select(clientfd + 1, &readset, NULL, NULL, NULL);

        if(FD_ISSET(STDIN_FILENO, &readset)){
            //清空缓冲区
            memset(buff, 0, sizeof(buff));
            //从标准输入缓冲区获取数据
            int ret = read(STDIN_FILENO, buff, sizeof(buff));
            if(ret == 0)    break;  //按下crtl + D
            printf("read stdin %d bytes, ", ret);
            //发送给对端
            ret = send(clientfd, buff, strlen(buff), 0);
            printf("send %d bytes.\n",ret);
        }

        if(FD_ISSET(clientfd, &readset)){
            //清空缓冲区
            memset(buff, 0, sizeof(buff));
            //从对端获取数据
            int ret = recv(clientfd, buff, sizeof(buff), 0);
            if(ret == 0)    break;    //recv的返回值为0,说明连接已经断开
            printf("recv msg from server: %s", buff);
        }
    }
    printf("对方已离开聊天室.\n");

    close(clientfd);

    return 0;
}
