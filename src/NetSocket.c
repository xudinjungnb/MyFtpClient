#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "NetSocket.h"

//	NetSocket，创建socket、初始化通信地址、绑定、监听、连接
NetSocket* init_ns(int type,short port,const char* ip,bool is_ser)
{
	NetSocket* ns = malloc(sizeof(NetSocket));
	ns->type = type;
    // 创建socket
	ns->sock_fd = socket(AF_INET,type,0);
	if(0 > ns->sock_fd)
	{
		perror("init_ns:socket");
		free(ns);
		return NULL;
	}

	//	初始化通信地址
	ns->addr.sin_family = AF_INET;
	ns->addr.sin_port = htons(port);
	ns->addr.sin_addr.s_addr = inet_addr(ip);
	ns->addrlen = sizeof(ns->addr);

	ns->is_ser = is_ser;

	//	TCP服务端 需要绑定+监听
    //	TCP客户端 需要连接
    //	UDP服务端 需要绑定
	if(is_ser)//是服务端
	{
		if(bind(ns->sock_fd,(SP)&ns->addr,ns->addrlen))
		{
			perror("init_ns:bind");
			free(ns);
			return NULL;
		}
		
		//	是TCP服务端
		if(SOCK_STREAM == ns->type)
		{
			if(listen(ns->sock_fd,10))
			{
				perror("init_ns:listen");
				free(ns);
				return NULL;
			}
		}
	}
    //是TCP客户端
	else if(SOCK_STREAM == ns->type)
	{
		if(connect(ns->sock_fd,(SP)&ns->addr,ns->addrlen))
		{	
			perror("init_ns:connect");
			free(ns);
			return NULL;		
		}
	}
	return ns;
}

//	TCP服务器的等待连接
NetSocket* accept_ns(NetSocket* ser_ns)
{
    //不是TCP协议 或者 是客户端直接退出（只有TCP服务器需要等待连接）
	if(SOCK_STREAM != ser_ns->type || false == ser_ns->is_ser)
	{
		printf("type use:SOCK_STREAM  is_ser use:true\n");
		return NULL;
	}

	//	申请连接者的NetSocket
	NetSocket* cli_ns = malloc(sizeof(NetSocket));
	cli_ns->type = ser_ns->type;
	cli_ns->is_ser = true;
	cli_ns->sock_fd = accept(ser_ns->sock_fd,(SP)&cli_ns->addr,&cli_ns->addrlen);
	if(0 > cli_ns->sock_fd)
	{
		perror("accept_ns:accept");
		free(cli_ns);
		return NULL;
	}
	return cli_ns;
}

//	发送数据 send/sendto
int send_ns(NetSocket* ns,const void* buf,size_t len)
{
	int ret = 0;//实际发送的字节数
    // 是TCP协议
	if(SOCK_STREAM == ns->type)
	{
		ret = send(ns->sock_fd,buf,len,0);
		if(0 > ret)
		{
			perror("send_ns:send");
		}
	}
    // 是UDP协议
	else
	{
		ret = sendto(ns->sock_fd,buf,len,0,(SP)&ns->addr,ns->addrlen);
		if(0 > ret)
		{
			perror("send_ns:sendto");	
		}
	}
	return ret;
}

//	接收数据
int recv_ns(NetSocket* ns,void* buf,size_t len)
{
	int ret = 0;//实际接收到的字节数
    // 是TCP协议
	if(SOCK_STREAM == ns->type)
	{
		ret = recv(ns->sock_fd,buf,len,0);
		if(0 > ret)
		{
			perror("recv_ns:recv");
		}
	}
    // 是UDP协议
	else
	{
        // addr是服务端通信地址
		ret = recvfrom(ns->sock_fd,buf,len,0,(SP)&ns->addr,&ns->addrlen);
		if(0 > ret)
		{
			perror("recv_ns:recv");	
		}
	}
	return ret;
	
}

//	关闭socket，释放NetSocket
void close_ns(NetSocket* ns)
{
	close(ns->sock_fd);
	free(ns);
	ns=NULL;
}
