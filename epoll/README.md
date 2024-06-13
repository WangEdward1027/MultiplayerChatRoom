多人聊天室 (C++多线程、Socket网络编程 小项目): <br>
该项目是用Xshell连接ubuntu虚拟机写的, Linux系统

注意事项: <br>
1.客户端的IP地址写死了(client.cpp中64行左右), 自己用的时候,记得把IP地址改成自己Xshell的IP地址。 <br>
2.编译的shell命令: <br>
g++ chatServer.cpp -o server <br>
g++ client.cpp -o client -pthread <br>


