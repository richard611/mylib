#ifndef __THREADMANAGER_H_
#define __THREADMANAGER_H_
#include "cz_mrsw_lock.h"

//线程控制锁类声明和实现，完全采用内联模型，保证效率
class CThreadManager {
private:
//大家可能注意到，线程控制锁，大多数情况下用于查询，
//只有在线程起停时，才会改写内部的值，
//因此，此处用的全部是单写多读变量
	CMRSWbool m_bThreadContinue; //线程持续的标志
	CMRSWint m_nThreadCount; //线程计数器
//很多时候，我们做测试代码，可能需要多个线程并发
//在Debug 打印输出时，可能会需要一个独立的线程ID，
//线程ID 需要做唯一性分配，因此，将这个分配器做在线程安全锁中，能被所有线程看到
//与业务无关，仅用于区别打印信息来自于哪个线程，
//这里提供一个线程ID 提供器，程序员可以根据需要，在线程中使用，
//这算是一个内嵌的debug 友好功能。
	CMRSWint m_nThreadID;
public:
	CThreadManager() {
	} //由于多线程安全变量对象内置初始化，此处无需初始化
	~CThreadManager() {
		CloseAll();
	} //退出时自动关闭所有线程
//启动逻辑，其实也是初始化逻辑
//在使用线程控制锁前，请一定要先调用本接口函数
	void Open(void) {
		CloseAll(); //为防止多重启动，先执行一次Close
//初始化线程控制变量
		m_bThreadContinue.Set(true);
		m_nThreadCount.Set(0);
	} //关闭所有线程逻辑
	void CloseAll(void) { //这个逻辑已经出现几次，此处不再赘述
		m_bThreadContinue.Set(false);
		while (m_nThreadCount.Get()) {
			TonyXiaoMinSleep();
		}
	} //启动一个线程前，线程计数器+1的动作
	int AddAThread(void) {
		return m_nThreadCount.Add();
	}
//线程退出前，线程计数器-1 动作
	void DecAThread(void) {
		m_nThreadCount.Dec();
	}
//查询线程维持变量的值
	bool ThreadContinue(void) {
		return m_bThreadContinue.Get();
	}
//获得线程计数器的值
	int GetThreadCount(void) {
		return m_nThreadCount.Get();
	}
//分配一个线程ID，供Debug 使用
	int GetID(void) {
		return m_nThreadID.Add() - 1;
	}
};
#endif
