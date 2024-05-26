//客户端代码,编译要加 -pthread
#include <cstdio>
#include <iostream> 
#include <cstring>       //memset()的头文件
#include <sys/socket.h>  //socket(),connect()等函数的头文件
#include <netinet/in.h>  //sockaddr_in的头文件
#include <arpa/inet.h>   //inet_pton()函数的头文件
#include <unistd.h>      //close()函数的头文件
#include <pthread.h>     //pthread创建线程和管理线程的头文件
using namespace std;

#define BUF_SIZE 1024
char szMsg[BUF_SIZE];

//发送消息
void* SendMsg(void *arg){
    int sock = *((int*)arg);
    while(1){
        //scanf("%s",szMsg);
        fgets(szMsg,BUF_SIZE,stdin); //使用fgets代替scanf
        if(szMsg[strlen(szMsg) - 1] == '\n'){
            szMsg[strlen(szMsg)- 1] = '\0'; //去除换行符
        }
        
        if(!strcmp(szMsg,"QUIT\n") || !strcmp(szMsg,"quit\n")){
            close(sock);
            exit(0);
        }
        send(sock, szMsg, strlen(szMsg), 0);
    }
    return nullptr;
}

//接收消息
void* RecvMsg(void * arg){
    int sock = *((int*)arg);
    char msg[BUF_SIZE];
    while(1){
        int len = recv(sock, msg, sizeof(msg)-1, 0);
        if(len == -1){
            cout << "系统挂了" << endl;
            return (void*)-1;
        }
        msg[len] = '\0';
        printf("%s\n",msg);
    }
    return nullptr;
}

int main()
{
    //创建socket
    int hSock;
    hSock = socket(AF_INET, SOCK_STREAM, 0);
    if(hSock < 0){
        perror("socket creation failed");
        return -1;
    }

    //绑定端口
    sockaddr_in servAdr;
    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_port = htons(9999);
    if(inet_pton(AF_INET, "172.16.51.88", &servAdr.sin_addr) <= 0){
        perror("Invalid address");
        return -1;
    }
    
    //连接到服务器
    if(connect(hSock, (struct sockaddr*)&servAdr, sizeof(servAdr)) < 0){
        perror("连接服务器失败");
        cout << "请检查是否已启动服务器。" << endl;
        return -1;
    }else{
        printf("已连接到服务器，IP地址：%s，端口：%d\n", inet_ntoa(servAdr.sin_addr), ntohs(servAdr.sin_port));
        printf("欢迎来到私人聊天室,请输入你的聊天用户名:");
    }
    
    //创建线程
    pthread_t sendThread,recvThread;
    if(pthread_create(&sendThread, NULL, SendMsg, (void*)&hSock)){
        perror("创建发送消息线程失败");
        return -1;
    }
    if(pthread_create(&recvThread, NULL, RecvMsg, (void*)&hSock)){
        perror("创建接收消息线程失败");
        return -1;
    }

    //等待线程结束
    pthread_join(sendThread, NULL);
    pthread_join(recvThread, NULL);

    //关闭socket
    close(hSock);

    return 0;
}
