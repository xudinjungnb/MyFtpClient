#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <math.h>
#include "NetSocket.h"
#include "getch.h"
#include "ftp_client.h"
#include "tools.h"

//只能在本文件使用的静态函数recv_status、send_cmd、pasv_FTPClient

//	1、接收命令结果并判断 is_end决定出错时是否直接退出程序
static int recv_status(FTPClient* ftp,int status,bool is_end)
{
    // 接收命令结果
	size_t size = recv((ftp->cli_sock_ns)->sock_fd,ftp->buf,BUF_SIZE,0);	
	if(0 >= size)
	{
		printf("与FTP服务器断开连接...\n");	
		return EXIT_FAILURE;
	}
	// 打印结果
	ftp->buf[size] = '\0';//防止打印之前残留的内容
	printf("%s",ftp->buf);
	// 读出结果码
	int ret_status = 0;
	sscanf(ftp->buf,"%d",&ret_status);
	// 比较结果码
	if(ret_status != status && is_end)//直接退出程序
	{
		exit(EXIT_FAILURE);
	}
	return ret_status == status ? EXIT_SUCCESS:EXIT_FAILURE;//不直接退出程序
}

//	2、发送命令
static void send_cmd(FTPClient* ftp)
{
	int ret = send((ftp->cli_sock_ns)->sock_fd,ftp->buf,strlen(ftp->buf),0);	
	if(0 > ret)
	{
		printf("与FTP服务器断开连接...\n");	
		ERROR("send");
		return;
	}
}

//	3、开启PASV
static int pasv_FTPClient(FTPClient* ftp)
{
    // 发送命令
	sprintf(ftp->buf,"PASV\n");
	send_cmd(ftp);
    // 接收服务器返回信息
	if(recv_status(ftp,227,false)) return EXIT_FAILURE;

	uint8_t ip1,ip2,ip3,ip4;
	char ip[16] = {};
	uint16_t port1,port2,port;
	// 读出端口号
	sscanf(ftp->buf,"227 Entering Passive Mode (%hhu,%hhu,%hhu,%hhu,%hu,%hu)",&ip1,&ip2,&ip3,&ip4,&port1,&port2);
	// 获得ip、计算新的端口号
	sprintf(ip,"%hhu.%hhu.%hhu.%hhu",ip1,ip2,ip3,ip4);
	port = port1*256+port2;
	// 获得新的socket描述符
    ftp->cli_pasv_ns=init_ns(SOCK_STREAM,port,ip,false);//TCP客户端连接
	if(NULL==ftp->cli_pasv_ns)
	{
		ERROR("init_ns");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}


//	创建FTP客户端对象
FTPClient* create_FTPClient(void)
{
	FTPClient* ftp = malloc(sizeof(FTPClient));	

	ftp->cli_sock_ns = NULL;
	ftp->cli_pasv_ns=NULL;
	ftp->buf = malloc(BUF_SIZE);
	return ftp;
}

//	销毁FTP客户端对象
void destroy_FTPClient(FTPClient* ftp)
{
	close_ns(ftp->cli_sock_ns);//关闭控制连接
	free(ftp->buf);					 //释放缓冲区
	free(ftp);			 				//释放对象
}

//	登录连接FTP服务器
int connect_FTPClient(FTPClient* ftp,const char* ip,uint16_t port)
{
    ftp->cli_sock_ns=init_ns(SOCK_STREAM,port,ip,false);//TCP客户端连接
    if(NULL==ftp->cli_sock_ns)
    {
    		ERROR("init_ns");
    	return EXIT_FAILURE;
    }
	// 接收服务器返回信息
	recv_status(ftp,220,true);

	return EXIT_SUCCESS;
}

//	向服务器发送用户名
void user_FTPClient(FTPClient* ftp,const char* user)
{
	sprintf(ftp->buf,"USER %s\n",user);	
	send_cmd(ftp);
	recv_status(ftp,331,true);
}

//	向服务器发送密码
void pass_FTPClient(FTPClient* ftp,const char* pass)
{
	sprintf(ftp->buf,"PASS %s\n",pass);	
	send_cmd(ftp);
	recv_status(ftp,230,true);
}

//	pwd
void pwd_FTPClient(FTPClient* ftp)
{
	sprintf(ftp->buf,"PWD\n");	
	send_cmd(ftp);
	recv_status(ftp,257,false);
	
	//	记录服务器的当前工作路径
    //	%*d表示匹配一个整数并丢弃
	sscanf(ftp->buf,"%*d \"%s",ftp->server_path);
	*strchr(ftp->server_path,'\"') = '\0';//strchr() 用于查找字符串中的一个字符，并返回该字符在字符串中第一次出现的位置
}

//	cd
void cd_FTPClient(FTPClient* ftp,const char* path)
{
	sprintf(ftp->buf,"CWD %s\n",path);
	send_cmd(ftp);
	recv_status(ftp,250,false);

	pwd_FTPClient(ftp);//更新客户端记录的服务器当前工作路径
}

//	mkdir
void mkdir_FTPClient(FTPClient* ftp,const char* dir)
{
	sprintf(ftp->buf,"MKD %s\n",dir);	
	send_cmd(ftp);
	recv_status(ftp,257,false);
}

//	rmdir
void rmdir_FTPClient(FTPClient* ftp,const char* dir)
{
	sprintf(ftp->buf,"RMD %s\n",dir);	
	send_cmd(ftp);
	recv_status(ftp,250,false);
}

//	bye
void bye_FTPClient(FTPClient* ftp)
{
	sprintf(ftp->buf,"QUIT\n");	
	send_cmd(ftp);
	recv_status(ftp,221,false);
	destroy_FTPClient(ftp);
	exit(EXIT_SUCCESS);
}

//	ls
void ls_FTPClient(FTPClient* ftp)
{
	if(pasv_FTPClient(ftp)) return;//被动模式若开启数据通道失败，直接return结束
    // 此时已经有两个TCP连接
	// 发命令
	sprintf(ftp->buf,"LIST -al\n");	
	send_cmd(ftp);
    // 接收反馈数据
	if(recv_status(ftp,150,false))//若结果状态码不匹配，关闭数据通道，return结束
	{
		close_ns(ftp->cli_pasv_ns);
		return;
	}

	//	接收数据
	file_oi((ftp->cli_pasv_ns)->sock_fd,STDOUT_FILENO);	//	写入标准输出文件
	close_ns(ftp->cli_pasv_ns);

	//	接收执行结果
	recv_status(ftp,226,false);
}

//	get
void get_FTPClient(FTPClient* ftp,const char* file)
{
    // 先检查服务端目标文件是否存在
	sprintf(ftp->buf,"SIZE %s\n",file);	
	send_cmd(ftp);
	if(recv_status(ftp,213,false))
	{
		printf("待下载%s文件不存在\n",file);
		return;
	}
    // 记录服务端文件大小
    size_t ser_file_size = 0;
	sscanf(ftp->buf,"213 %u",&ser_file_size);
	//在本地创一个空文件
	int file_fd = open(file,O_WRONLY|O_CREAT,0644);
	if(0 > file_fd)
	{
		printf("当前目录没有写权限,下载失败\n");
		return;
	}
	
	if(pasv_FTPClient(ftp))
	 {
	 	close(file_fd);
		return;
	 }
	sprintf(ftp->buf,"RETR %s\n",file);	
	send_cmd(ftp);
	if(recv_status(ftp,150,false))
	{
		close_ns(ftp->cli_pasv_ns);
		close(file_fd);
		return;
	}
	 
	 
	 file_oi((ftp->cli_pasv_ns)->sock_fd,file_fd);
	 
     close_ns(ftp->cli_pasv_ns);
	close(file_fd);
	recv_status(ftp,226,false);
}

//	获取文件的最后修改时间
int get_file_mdtm(const char* file,char* mdtm)
{
	struct stat stat_buf;
	if(stat(file,&stat_buf)) return -1;
	
	//	转换得到最后修改时间结构体
	struct tm* t_m = localtime(&stat_buf.st_mtim.tv_sec);
	if(NULL == t_m)	return -1;

	sprintf(mdtm,"%04d%02d%02d%02d%02d%02d",
		t_m->tm_year+1900,
		t_m->tm_mon+1,
		t_m->tm_mday,
		t_m->tm_hour,
		t_m->tm_min,
		t_m->tm_sec);
	return 0;
}

//	获取文件大小
size_t get_file_size(int filefd)
{
	struct stat stat_buf;
	if(fstat(filefd,&stat_buf)) return -1;
	
	return stat_buf.st_size;
}

//	断点续传的上传命令
void put_FTPClient(FTPClient* ftp,const char* file)
{
	//	本地文件若不存在，直接return返回
	int file_fd = open(file,O_RDONLY);
	if(0 > file_fd)
	{
		printf("%s文件不存在，上传失败\n",file);
		return;
	}
    
    char ser_flie_mdtm[15] = {};//	用于保存服务器文件最后修改时间
	char cli_flie_mdtm[15] = {};//	用于保存客户端文件最后修改时间
    size_t ser_file_size = 0;	//	用于记录服务器中同名文件大小 
    size_t cli_file_size = get_file_size(file_fd);	//获得客户端同名文件大小
    //	获取客户端文件最后修改时间
	get_file_mdtm(file,cli_flie_mdtm);

    //	检查服务器是否存在同名文件
	sprintf(ftp->buf,"SIZE %s\n",file);
	send_cmd(ftp);
	if(!recv_status(ftp,213,false))//若存在同名文件
	{
		//	获得服务器中同名文件大小     在结果状态码后面会跟着同名文件大小
		sscanf(ftp->buf,"213 %u",&ser_file_size);

		//	检查服务器中该文件与本地中文件最后修改时间是否一致（不一致说明不是我传的文件）
		sprintf(ftp->buf,"MDTM %s\n",file);
		send_cmd(ftp);
		recv_status(ftp,213,false);
		sscanf(ftp->buf,"%*d %s",ser_flie_mdtm);
		printf("ser_flie_mdtm:%s\n",ser_flie_mdtm);
		if(0 == strcmp(ser_flie_mdtm,cli_flie_mdtm))//1、最后修改时间一致
		{
			//	根据文件大小决定是否断点续传
			if(ser_file_size == cli_file_size)//若大小相同，直接上传结束
			{
				printf("上传结束\n");
				close(file_fd);
				return;
			}
			else//若大小不相同，需要断点续传，而不是直接覆盖(而且只有服务端文件大小小于客户端文件这一种可能)
			{
				printf("开始断点续传\n");
				sprintf(ftp->buf,"REST %u\n",ser_file_size);//定位到断点
				send_cmd(ftp);
				recv_status(ftp,350,false);
				lseek(file_fd,ser_file_size,SEEK_SET);//定位到断点
			}
		}
		else//2、最后修改时间不一致
		{
			printf("服务器存在同名文件，是否覆盖(y/n)?");
			int cmd = getchar();
			if('y' != cmd)
			{
				printf("终止上传\n");
				close(file_fd);
				return;
			}
		}
	}
	
    // 开始正常上传
	if(pasv_FTPClient(ftp))//开启数据通道
	{
		close(file_fd);
		return;
	}

	//	开始上传
	sprintf(ftp->buf,"STOR %s\n",file);	
	send_cmd(ftp);
	if(recv_status(ftp,150,false))
	{
		close(file_fd);
		close_ns(ftp->cli_pasv_ns);
		return;
	}
    //	零拷贝 若是断点续传，此刻file_fd的文件指针已经被定位到断点
	sendfile(file_fd,(ftp->cli_pasv_ns)->sock_fd,NULL,cli_file_size-ser_file_size);

	close(file_fd);
	close_ns(ftp->cli_pasv_ns);
	recv_status(ftp,226,false);

	//	上传文件的最后修改时间
	sprintf(ftp->buf,"MDTM %s %s/%s\n",cli_flie_mdtm,ftp->server_path,file);
	printf("-----%s\n",ftp->buf);
	send_cmd(ftp);
	recv_status(ftp,213,false);
}


