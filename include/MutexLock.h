#ifndef __MUTEXLOCK_H_
#define __MUTEXLOCK_H_

#ifdef WIN32 //Windows 下定义
//定义锁变量类型
#define MUTEX CRITICAL_SECTION
//定义初始化锁的功能
#define MUTEXINIT(m) InitializeCriticalSection(m)
//定义加锁
#define MUTEXLOCK(m) EnterCriticalSection(m)
//定义解锁
#define MUTEXUNLOCK(m) LeaveCriticalSection(m)
//定义摧毁锁变量操作
#define MUTEXDESTROY(m) DeleteCriticalSection(m)
#else //Linux 下定义
//定义锁变量类型
#define MUTEX pthread_mutex_t
//定义初始化锁的功能
#define MUTEXINIT(m) pthread_mutex_init(m, NULL) //TODO: check error
//定义加锁
#define MUTEXLOCK(m) pthread_mutex_lock(m)
//定义解锁
#define MUTEXUNLOCK(m) pthread_mutex_unlock(m)
//定义摧毁锁变量操作
#define MUTEXDESTROY(m) pthread_mutex_destroy(m)
#endif

class CMutexLock {

	
public:
	CMutexLock(void) {
		MUTEXINIT(&m_Lock);
	} //构造函数，初始化锁
	~CMutexLock(void) {
		MUTEXDESTROY(&m_Lock);
	} //析构函数，摧毁锁
public:
	void Lock() {
		MUTEXLOCK(&m_Lock);
	} //加锁操作
	void Unlock() {
		MUTEXUNLOCK(&m_Lock);
	} //解锁操作
private:
	MUTEX m_Lock; //锁变量（私有）
};

#endif

