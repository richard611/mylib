#include "CSocket.h"
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <iostream>
using namespace std;

CTonyLowDebug *g_pSocketDebug = NULL;

SocketBuilder::SocketBuilder(CTonyLowDebug *pDebug, CTonyMemoryPoolWithLock *pPool) {
	assert(pDebug != NULL);
	assert(pPool != NULL);

	g_pSocketDebug = pDebug;
	m_pPool = pPool;
}

ServerSocket* SocketBuilder::createTCPServerSocket(uint16_t port) {
	ServerSocket *sock = new ServerSocket(port);
	//regist the socket
	if (!sock->isValid()) {
		SOCKET_DEBUG("create server socket[port = %d] failt", port);
		return sock;
	} 

	m_pPool->RegisterSocket(sock->getSocketFd(), "server socket");

	return sock;
}

Socket* SocketBuilder::createSocket(int type, uint16_t port) {
	Socket *sock = NULL;// new Socket(port, m_pDebug);
	if (SOCK_STREAM == type) {
		sock = new TcpSocket(port);
		m_pPool->RegisterSocket(sock->getSocketFd(), "tcp");
	} else {
		sock = new UdpSocket(port);
		m_pPool->RegisterSocket(sock->getSocketFd(), "udp");
	}

	return sock;
}

void SocketBuilder::close(Socket *sock) {
	m_pPool->CloseSocket(sock->getSocketFd());
	delete sock;
}

int Socket::connect(const char * const ip, const uint16_t port) {
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(port);

	return ::connect(fd, (struct sockaddr*)&addr, sizeof(addr));
}

TcpSocket::TcpSocket(uint16_t port) {
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	
	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		SOCKET_DEBUG("bind error");	
	}
}

TcpSocket::TcpSocket(int fd) {
	this->fd = fd;
}

TcpSocket::~TcpSocket() {
	if (fd > 0) close(fd);
}

int TcpSocket::send(void *buffer, int length) {
	if (buffer == NULL || length <= 0) {
		SOCKET_DEBUG("SOCKET: send param error \n");
	}

	int curSendLength = 0;//当前已经发送的数据的长度
	//设置读取的延时时间,在当前时间内无法读取数据则抛出异常
	struct timeval delay = {5, 0};

	//使用select机制来做数据发送超时处理
	fd_set sendSet;
	fd_set _sendSet;
	FD_ZERO(&sendSet);
	FD_SET(fd, &sendSet);

	do {
		_sendSet = sendSet;
		//select超时,亦也可能出错
		if (select(fd+1, NULL, &_sendSet, NULL, &delay) < 0) {
			SOCKET_DEBUG("SOCKET: send data out time \n");
			break;
		}

		if (FD_ISSET(fd, &_sendSet)) {
			int sendLength = ::send(fd, (char *)buffer + curSendLength,
					length - curSendLength, 0);
			//非阻塞的时候发送失败可能是由于缓冲区满，此时只需继续等待
			if (sendLength < 0 &&
					(ENOBUFS == errno || EWOULDBLOCK == errno)) {
				continue;
			} else if (sendLength > 0) {
				curSendLength += sendLength;
			} else {
				SOCKET_DEBUG("SOCKET:send data error \n");
				break;
			}
		}
		
	} while(curSendLength < length);

	return curSendLength - length; //如果<0则表示发送出现异常 =0为正确
}

int TcpSocket::recv(void *buffer, int length) {
	if (NULL == buffer && length < 0 ) return -1;

	int curRecvLength = 0;

	struct timeval delay = {5, 0};

	fd_set recvSet;
	fd_set _recvSet;
	FD_ZERO(&recvSet);
	FD_SET(fd, &recvSet);

	do {
		_recvSet = recvSet;
	
		if (select(fd+1, &recvSet, NULL, NULL, &delay) < 0) break;
		if (FD_ISSET(fd, &_recvSet)) {
			int recvLength = ::recv(fd, (char *)buffer + curRecvLength,
					length - curRecvLength, 0);
			if (recvLength < 0 && EWOULDBLOCK == errno ) {	//缓冲区没有数据
				continue;
			} else if (recvLength > 0) {
				curRecvLength += recvLength;
			} else {
				break;
			}
		}
	} while(curRecvLength < length);

	//如果全部接收完成，则返回数据长度，否则返回-1
	int ret = (curRecvLength - length) == 0 ? curRecvLength : -1;
	return ret;
}

UdpSocket::UdpSocket(uint16_t port) {
	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);	
	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		SOCKET_DEBUG("bind error");	
	}

}

UdpSocket::UdpSocket(int fd) {
	this->fd = fd;
}

UdpSocket::~UdpSocket() {
	if (fd > 0) close(fd);
}

int UdpSocket::send(void *buffer, int length) {
	return ::send(fd, buffer, length, 0);
}

int UdpSocket::recv(void *buffer, int length) {
	return ::recv(fd, buffer, length, 0);
}

ServerSocket::ServerSocket(uint16_t port) : TcpSocket(port) {
	listen(fd, 10);
}

Socket* ServerSocket::accept(struct sockaddr *addr) {
	int socklen = 0;
	int clientFd =  ::accept(fd, addr, (socklen_t *)(&socklen));
	Socket* sock = new TcpSocket(clientFd);

	return sock;
}
