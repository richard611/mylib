#ifndef __MRSWLOCK_H_
#define __MRSWLOCK_H_

#include "MutexLock.h"

#ifndef ULONG
#define ULONG unsigned long
#endif
//定义结构体的习惯：typedef 定义出一种新的，针对本结构体的变量类型，方便后续使用
typedef struct _TONY_XIAO_MULTI_READ_SINGLE_WRITE_LOCK_ { //注意变量命名，遵循匈牙利命名法在类中的命名规则
	int m_nReadCount; //读计数器
	bool m_bWriteFlag; //写标志
	MUTEX m_Lock; //作为单写多读锁本身，也应该是多线程安全的，
//因此，需要增加一个锁变量
} STonyXiaoMultiReadSingleWriteLock; //这是新的变量类型，可以定义变量
//习惯性做法，定义结构体后，马上定义其尺寸常量，方便后续申请内存方便
const ULONG STonyXiaoMultiReadSingleWriteLockSize =
		sizeof(STonyXiaoMultiReadSingleWriteLock);

//高精度睡眠函数，采用内联模式加速调用
inline void TonyXiaoMinSleep(void) {
#ifdef WIN32
	Sleep(1); //Windows 下不做改变，沿用应用级Sleep 函数
#else // not WIN32
	//Linux 下，使用nanosleep 实现高精度睡眠
	struct timespec slptm;
	slptm.tv_sec = 0;
	slptm.tv_nsec = 1000; //1000 ns = 1 us
	if (nanosleep(&slptm, NULL) == -1)
		usleep(1);
#endif //WIN32
}
//Linux 下普通的睡眠函数对比
#ifndef __Sleep
#define Sleep(ms) usleep(ms*1000)
#endif
//大家可以看看笔者的函数命名习惯，基本遵循匈牙利命名法，即单词首字母大写
//MRSW 是Multi Read and Signal Write（多读和单写）的缩写
//MRSWLock 前缀表示单写多读锁
//中间一个“_”分割符，后面是函数的功能描述Greate，创建，Destroy，摧毁，等等
void MRSWLock_Create(STonyXiaoMultiReadSingleWriteLock* pLock) ;
void MRSWLock_Destroy(STonyXiaoMultiReadSingleWriteLock* pLock) ;

//获取写状态
bool MRSWLock_GetWrite(STonyXiaoMultiReadSingleWriteLock* pLock) ;
//获取读计数器
int MRSWLock_GetRead(STonyXiaoMultiReadSingleWriteLock* pLock) ;

//进入写操作函数
void MRSWLock_EnableWrite(STonyXiaoMultiReadSingleWriteLock* pLock) ;
void MRSWLock_DisableWrite(STonyXiaoMultiReadSingleWriteLock* pLock) ;

//进入读函数，返回当前的读计数器值
int MRSWLock_AddRead(STonyXiaoMultiReadSingleWriteLock* pLock) ;

//返回读计数器变化后的结果
int MRSWLock_DecRead(STonyXiaoMultiReadSingleWriteLock* pLock) ;
void MRSWLock_Read2Write(STonyXiaoMultiReadSingleWriteLock* pLock) ;
///////////////////////////////////////////////////////////////////////////
class CMultiReadSingleWriteLock {
public:
//构造函数和析构函数，自动调用结构体的初始化和摧毁
	CMultiReadSingleWriteLock() {
		MRSWLock_Create(&m_Lock);
	}
	~CMultiReadSingleWriteLock() {
		MRSWLock_Destroy(&m_Lock);
	}
public:
//相应的公有方法，完全是调用C 语言的函数
	void EnableWrite(void) {
		MRSWLock_EnableWrite(&m_Lock);
	}
	void DisableWrite(void) {
		MRSWLock_DisableWrite(&m_Lock);
	}
	void Read2Write(void) {
		MRSWLock_Read2Write(&m_Lock);
	}
	void DecRead(void) {
		MRSWLock_DecRead(&m_Lock);
	}
	void AddRead(void) {
		MRSWLock_AddRead(&m_Lock);
	}
	bool GetWrite(void) {
		MRSWLock_GetWrite(&m_Lock);
	}
	int GetRead(void) {
		MRSWLock_GetRead(&m_Lock);
	}
private:
//私有结构体
	STonyXiaoMultiReadSingleWriteLock m_Lock;
};
//////////////////////////////////////////////////////////////
//这里是类声明
class CMRSWint {
public:
	CMRSWint();
	~CMRSWint() {
	} //析构函数不做任何事
public:
	int Get(void);
	int Set(int nValue);
	int Add(int nValue = 1);
	int Dec(int nValue = 1);
	int GetAndClean2Zero(void);
	int DecUnless0(int nValue = 1); //如果不是，-1
private:
	int m_nValue;
	CMultiReadSingleWriteLock m_Lock;
};

/////////////////////////////////////////////////////////
//bool 变量的实现更加简单
class CMRSWbool {
public:
	CMRSWbool() {
	}
	~CMRSWbool() {
	}
public:
	//得到变量的值，或者设置变量的值，均调用整型对象完成
	bool Get(void) {
		return (bool) m_nValue.Get();
	}
	bool Set(bool bFlag) {
		return m_nValue.Set((int) bFlag);
	}
private:
	CMRSWint m_nValue; //内部聚合一个上文定义的整型单写多读锁安全变量
};		




#endif
