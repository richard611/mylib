#ifndef SOCKET_H_
#define SOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>

#include "std.h"
#include "MemoryManager.h"
#include "TonyLowDebug.h"

//CTonyLowDebug *g_pSocketDebug = NULL;
#define SOCKET_DEBUG g_pSocketDebug->Debug2File

class Socket;
class ServerSocket;

//socket创建类
//所有socket对象都应该由该对象创建
class SocketBuilder {
public:
	//@param CTonyLowDebug *pDebug 调试对象
	//@param CTonyMemoryPoolWithLock *pPool 内存池，用于管理跟踪fd
	SocketBuilder(CTonyLowDebug *pDebug, CTonyMemoryPoolWithLock *pPool);
	
	//创建tcp服务器端的socket
	//@param uint16_t port socket端口
	//@return ServerSocket对象指针
	ServerSocket* createTCPServerSocket(uint16_t port);

	//创建UDP或者TCP socket对象
	//@param int type SOCK_STREAM或者SOCK_DGRAM
	//@param uint16_t port socket绑定的端口号，0表示随机分配
	//@return Socket对象指针
	Socket* createSocket(int type, uint16_t port = 0);

	void close(Socket *sock);

private:
	CTonyMemoryPoolWithLock *m_pPool;		//用于管理socket的fd
};
//Socket类
//封装了一些基本的socket函数操作
class Socket {
public:
	//@param int type socket类型
	//@param uint16_t port 端口号，默认随机
	/*
	Socket(int type, uint16_t port = 0);
	Socket(int fd);
	virtual ~Socket();
	*/
	int& getSocketFd() { return fd; };

	//发送数据
	//@param void *buffer 数据缓冲区
	//@param int length 数据长度
	//@return 发送数据的长度，-1为发送错误
	virtual int send(void *buffer, int length) = 0;

	//接收数据 尽可能的接受需要接受的数据长度
	//@param void *buffer 接受数据的缓冲区
	//@param int length 需要接收数据的长度
	//@return 实际接受数据的长度，小于零表示接受错误
	virtual int recv(void *buffer, int length) = 0;
	
	//连接到服务器
	//@param const char *const ip 服务器ip
	//@param const uint16_t port 服务器端口
	int connect(const char * const ip, const uint16_t port);
	
	//判断创建的socket是否有效
	bool isValid() const { return fd > 0 ? true : false; }

public:
	int fd;
	friend class ServerSocket;
};

//tcp socket
class TcpSocket : public Socket {
public:
	TcpSocket(uint16_t port);
	TcpSocket(int fd);
	virtual ~TcpSocket();

	virtual int send(void *buffer, int length);
	virtual int recv(void *buffer, int length);
};

//udp socket
class UdpSocket : public Socket {
public:
	UdpSocket(uint16_t port);
	UdpSocket(int fd);
	virtual ~UdpSocket();

	virtual int send(void *buffer, int length);
	virtual int recv(void *buffer, int length);
};

//服务器端的socket
class ServerSocket : public TcpSocket {
public:
	ServerSocket(uint16_t port);
	virtual ~ServerSocket(){};

	Socket* accept(struct sockaddr *addr);
};

#endif
