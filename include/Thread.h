#ifndef __THREAD_H_
#define __THREAD_H_
#ifdef WIN32 //Windows 下定义
#include <process.h>
//线程句柄类型
#define THREAD HANDLE
//线程ID 类型
#define THREADID unsigned
//线程启动函数统一构型，注意函数型宏的使用
#define THREADCREATE(func, args, thread, id) \
thread = (HANDLE)_beginthreadex(NULL, 0, func, (PVOID)args, 0, &id);
//线程启动失败后返回错误码定义
#define THREADCREATE_ERROR NULL
//线程函数标准构型
#define THREADFUNCDECL(func,args) unsigned __stdcall func(PVOID args)
//工程中通常需要检测本次开机以来的毫秒数，Windows 和Linux 不一样，此处予以统一。
#define _GetTimeOfDay GetTickCount
//Windows 下最小睡眠精度，经实测为10ms
#define MIN_SLEEP 10
#else //Linux 下定义
//线程句柄类型
#define THREAD pthread_t
//线程ID 类型
#define THREADID unsigned //unused for Posix Threads
//线程启动函数统一构型，注意函数型宏的使用
#define THREADCREATE(func, args, thread, id) \
pthread_create(&thread, NULL, func, args);
//线程启动失败后返回错误码定义
#define THREADCREATE_ERROR -1
//线程函数标准构型
#define THREADFUNCDECL(func,args) void * func(void *args)
//#define Sleep(ms) usleep((__useconds_t)(ms*1000))
//工程中通常需要检测本次开机以来的毫秒数，Windows 和Linux 不一样，此处予以统一。
unsigned long GetTickCount(void);
#include <sys/time.h>
#define _GetTimeOfDay GetTickCount
//Linux 下没有Sleep 函数，为了便于统一编程，仿照Windows 下定义该函数
#define Sleep(ms) usleep(ms*1000)
#define MIN_SLEEP 1
#endif//end of WIN32
/////////////////////////////////////////////////////////
#ifdef WIN32 //Windows 下定义
//Windows 下有此函数，无需重复定义
#else //Linux 下定义
//获得本次开机以来毫秒数
inline unsigned long GetTickCount(void) {
	unsigned long lRet = 0;
	struct timeval tv;
	gettimeofday(&tv, null);
	lRet = tv.tv_usec / 1000;
	return lRet;
}
#endif//end of WIN32
#endif//end of __THREAD_H_
