#ifndef NETWORK_H
#define NETWORK_H
#include <stdbool.h>
#include <stdio.h>
#include <netinet/in.h>

typedef struct NetSocket
{
	int type;					//	自己通信协议TCP/UDP
	int sock_fd;				//	自己的socket描述符
	bool is_ser;				//	自己是客户端还是服务器
    // UDP要用的
	struct sockaddr_in addr;	//	对面的通信地址
	socklen_t addrlen;			//	地址结构体字节数
}NetSocket;

typedef struct sockaddr* SP;

//	创建NetSocket，
//  创建socket、初始化通信地址、绑定、监听、连接
NetSocket* init_ns(int type,short port,const char* ip,bool is_ser);

//	TCP服务器的等待连接
NetSocket* accept_ns(NetSocket* ser_ns);

//	发送数据
int send_ns(NetSocket* ns,const void* buf,size_t len);

//	接收数据
int recv_ns(NetSocket* ns,void* buf,size_t len);

//	关闭socket，释放NetSocket
void close_ns(NetSocket* ns);

#endif//NETWORK_H
