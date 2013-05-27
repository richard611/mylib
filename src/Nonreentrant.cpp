#include "std.h"
#include "MutexLock.h"

#include "Nonreentrant.h"
//不可重入锁类实现
CNonReentrant::CNonReentrant() { //初始化内部变量
	m_bAlreadRunFlag = false;
} //核心的Set 函数
bool CNonReentrant::Set(bool bRunFlag) {
//请注意，返回结果是设置动作的成功与否，和内部的bool 变量，没有任何关系
	bool bRet = false;
	if (bRunFlag) //需要设置为真的逻辑，比较复杂
	{
		m_Lock.Lock(); //进入锁域
		{
			if (!m_bAlreadRunFlag) { //如果原值为“假”，表示可以设置
				m_bAlreadRunFlag = true; //设置内部bool 变量
				bRet = true; //返回真，表示设置成功
			} //否则，不做任何事，即内部bool 变量不做设置，且返回假
		}
		m_Lock.Unlock(); //退出锁域
	} else { //这是设置为“假”的情况
		m_Lock.Lock(); //进入锁域
		{
			m_bAlreadRunFlag = false; //无条件设置内部bool 变量为“假”
			bRet = true; //由于这也是设置成功，因此返回真
		}
		m_Lock.Unlock(); //退出锁域
	}
	return bRet;
}
