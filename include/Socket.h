#ifndef __SOCKET_H_
#define __SOCKET_H_
#ifdef WIN32 //Windows 下定义
#include <winsock.h>
//定义Socket 套接字变量类型
#define Linux_Win_SOCKET SOCKET
//标准的Socket 关闭函数，不过，由于后文资源池接管了关闭动作，此处隐去定义
//#define Linux_Win_CloseSocket closesocket
//协议族命名约定
#define Linux_Win_F_INET AF_INET
//非法的socket 表示值定义
#define Linux_Win_InvalidSocket INVALID_SOCKET
//标准socket 错误返回码
#define Linux_Win_SocketError SOCKET_ERROR
//setsockopt 第4 个变量类型定义
#define Linux_Win_SetSockOptArg4UseType const char
//getsockopt 第4 个变量类型定义
#define Linux_Win_GetSockOptArg4UseType char
//send recv 函数的最后一个参数类型
#define Linux_Win_SendRecvLastArg 0
//注意此处，所有的错误返回码定名，Windows 平台向向标准的伯克利socket 规范靠拢。
#define EWOULDBLOCK WSAEWOULDBLOCK //10035
#define EINPROGRESS WSAEINPROGRESS
#define EALREADY WSAEALREADY
#define ENOTSOCK WSAENOTSOCK
#define EDESTADDRREQ WSAEDESTADDRREQ
#define EMSGSIZE WSAEMSGSIZE
#define EPROTOTYPE WSAEPROTOTYPE
#define ENOPROTOOPT WSAENOPROTOOPT
#define EPROTONOSUPPORT WSAEPROTONOSUPPORT
#define ESOCKTNOSUPPORT WSAESOCKTNOSUPPORT
#define EOPNOTSUPP WSAEOPNOTSUPP
#define EPFNOSUPPORT WSAEPFNOSUPPORT
#define EAFNOSUPPORT WSAEAFNOSUPPORT
#define EADDRINUSE WSAEADDRINUSE
#define EADDRNOTAVAIL WSAEADDRNOTAVAIL
#define ENETDOWN WSAENETDOWN
#define ENETUNREACH WSAENETUNREACH
#define ENETRESET WSAENETRESET
#define ECONNABORTED WSAECONNABORTED //10053
#define ECONNRESET WSAECONNRESET //10054
#define ENOBUFS WSAENOBUFS
#define EISCONN WSAEISCONN
#define ENOTCONN WSAENOTCONN
#define ESHUTDOWN WSAESHUTDOWN
#define ETOOMANYREFS WSAETOOMANYREFS
#define ETIMEDOUT WSAETIMEDOUT
#define ECONNREFUSED WSAECONNREFUSED
#define ELOOP WSAELOOP
#define EHOSTDOWN WSAEHOSTDOWN
#define EHOSTUNREACH WSAEHOSTUNREACH
#define EPROCLIM WSAEPROCLIM
#define EUSERS WSAEUSERS
#define EDQUOT WSAEDQUOT
#define ESTALE WSAESTALE
#define EREMOTE WSAEREMOTE

#else //Linux 下定义
//定义Socket 套接字变量类型
#define Linux_Win_SOCKET int
//标准的Socket 关闭函数，不过，由于后文资源池接管了关闭动作，此处隐去定义
//#define Linux_Win_CloseSocket close
//协议族命名约定
#define Linux_Win_F_INET AF_INET
//非法的socket 表示值定义
#define Linux_Win_InvalidSocket -1
//标准socket 错误返回码
#define Linux_Win_SocketError -1
//setsockopt 第4 个变量类型定义
#define Linux_Win_SetSockOptArg4UseType void
//getsockopt 第4 个变量类型定义
#define Linux_Win_GetSockOptArg4UseType void
//send recv 函数的最后一个参数类型
#define Linux_Win_SendRecvLastArg MSG_NOSIGNAL
#endif//end of WIN32
#endif//end of __SOCKET_H_