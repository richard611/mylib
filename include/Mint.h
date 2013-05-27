#ifndef __MINT_H_
#define __MINT_H_
//这是.h 文件中的声明
#include "MutexLock.h"

typedef struct _MINT_ //这是整型的多线程安全单元
{
	int m_nValue; //这是整型值
	MUTEX m_MyLock; //这是实现保护的C 语言锁变量
} MINT, MBOOL; //int 型和bool 型同时实现
//初始化一个线程安全变量,同时可以设置值,返回设置值
extern int MvarInit(MINT& (mValue), int nValue = 0);
//摧毁一个线程安全变量
extern void MvarDestroy(MINT& (mValue));
//设置一个线程安全变量的值,返回设置的值
extern int MvarSet(MINT& (mValue), int nValue);
//得到一个线程安全变量的值
extern int MvarGet(MINT& (mValue));
//线程安全变量做加法运算,默认+1
extern int MvarADD(MINT& (mValue), int nValue = 1);
//线程安全变量做减法运算,默认-1
extern int MvarDEC(MINT& (mValue), int nValue = 1);
//这里是.cpp 文件中的实现
int MvarInit(MINT& mValue, int nValue) ;
void MvarDestroy(MINT& mValue) ;
int MvarSet(MINT& mValue, int nValue) ;
int MvarGet(MINT& mValue) ;
int MvarADD(MINT& mValue, int nValue) ;
int MvarDEC(MINT& mValue, int nValue) ;

//为了简化调用，笔者又定义了一批宏缩写
#define XMI MvarInit
#define XME MvarDestroy
#define XMG MvarGet
#define XMS MvarSet
#define XMA MvarADD
#define XMD MvarDEC

class CMint //int 型多线程安全变量类
{
public:
	CMint(int nVlaue=0) {XMI(m_nValue,nVlaue);}~CMint(void) {XME(m_nValue);}
public:
	int Get(void) {
		return XMG(m_nValue);
	}
	int Set(int nValue) {
		return XMS(m_nValue, nValue);
	}
	int Add(int nValue = 1) {
		return XMA(m_nValue, nValue);
	}
	int Dec(int nValue = 1) {
		return XMD(m_nValue, nValue);
	}
private:
	MINT m_nValue;
};

class CMbool //bool 型多线程安全的变量类
{
public:
	CMbool(bool nVlaue = false) {
		XMI(m_nValue, nVlaue);
	}
	~CMbool(void) {
		XME(m_nValue);
	}
public:
	//只提供Get 和Set 方法
	int Get(void) {
		return XMG(m_nValue);
	}
	int Set(bool nValue) {
		return XMS(m_nValue, nValue);
	}
private:
	MBOOL m_nValue;
};

#endif
