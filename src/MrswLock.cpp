#include "std.h"
#include "MutexLock.h"
#include "Thread.h"
#include "MrswLock.h"
//大家可以看看笔者的函数命名习惯，基本遵循匈牙利命名法，即单词首字母大写
//MRSW 是Multi Read and Signal Write（多读和单写）的缩写
//MRSWLock 前缀表示单写多读锁
//中间一个“_”分割符，后面是函数的功能描述Greate，创建，Destroy，摧毁，等等
void MRSWLock_Create(STonyXiaoMultiReadSingleWriteLock* pLock) {
	MUTEXINIT(&(pLock->m_Lock)); //初始化内部锁
	pLock->m_nReadCount = 0; //初始化读计数器
	pLock->m_bWriteFlag = false; //初始化写标志
}
void MRSWLock_Destroy(STonyXiaoMultiReadSingleWriteLock* pLock) {
	MUTEXLOCK(&pLock->m_Lock); //还记得前文的技巧吗？
	MUTEXUNLOCK(&pLock->m_Lock); //利用一次空加锁解锁动作，规避风险
	MUTEXDESTROY(&(pLock->m_Lock)); //摧毁内部锁
}

//获取写状态
bool MRSWLock_GetWrite(STonyXiaoMultiReadSingleWriteLock* pLock) {
	bool bRet = false;
	MUTEXLOCK(&(pLock->m_Lock));
	{
		bRet = pLock->m_bWriteFlag;
	}
	MUTEXUNLOCK(&(pLock->m_Lock));
	return bRet;
}
//获取读计数器
int MRSWLock_GetRead(STonyXiaoMultiReadSingleWriteLock* pLock) {
	int nRet = 0;
	MUTEXLOCK(&(pLock->m_Lock));
	{
		nRet = pLock->m_nReadCount;
	}
	MUTEXUNLOCK(&(pLock->m_Lock));
	return nRet;
}

//进入写操作函数
void MRSWLock_EnableWrite(STonyXiaoMultiReadSingleWriteLock* pLock) {
	while (1) //死循环等待
	{
		//第一重循环，检测“写”标志是否为“假”，尝试抢夺“写”权
		//请大家放心，即使有其他线程正在写，操作总会完成
		//因此，这个死循环不会永远死等，一般很快就会因为条件满足而跳出
		MUTEXLOCK(&(pLock->m_Lock)); //内部锁加锁，进入锁域
		{
			//在此域内，其他访问本单写多读锁的线程，会因为内部锁的作用被挂住
			//判断是否可以抢夺“写”权利
			if (!pLock->m_bWriteFlag) {
				//如果写标志为“假”，即可以抢夺“写”权利
				//注意，此时一定不要解内部锁，应该立即将写标志置为“真”
				pLock->m_bWriteFlag = true;
				//抢到“写”权后，本轮逻辑完毕，解除内部锁后，可以退出
				//关键：请注意，这里多一句解锁命令，为安全跳出使用
				MUTEXUNLOCK(&(pLock->m_Lock));
				//注意退出方式，使用goto 精确定位
				goto MRSWLock_EnableWrite_Wait_Read_Clean;
			} //这是if 域结束
			  //此处，“写”标志为真，表示其他线程已经抢到“写”权，
			  //本线程必须悬挂等待。
		} //这是锁域结束
		  //等待睡眠时，应该及时释放内部锁，避免其他线程被连锁挂死
		MUTEXUNLOCK(&(pLock->m_Lock));
		//这是一个特殊的Sleep，下面会讲到
		TonyXiaoMinSleep();
	}
	//这是第一阶段完成标志，程序运行到此，表示“写”权已经被本程序抢到手
	MRSWLock_EnableWrite_Wait_Read_Clean:
	//下面，开始等待其他的“读”操作完毕
	while (MRSWLock_GetRead(pLock)) //请务必关注这个调用
	//这是利用公有的取读状态方法来获取信息
	//前文已经说明，这个函数内部，是“资源锁”模型，
	//即函数内进行了内部锁加锁，是线程安全的，
	//退出函数，内部锁自动解锁
	//因此，本循环Sleep 期间，本函数没有挂住内部锁，
	//其他线程访问的其他公有方法，可以自由修改读计数器的值。
	//这个规避逻辑异常重要!
	{
		//第二重循环，等待所有的“读”操作退出
		//请放心，一旦“写”标志被置为“真”，新的“读”操作会被悬挂，不会进来
		//而老的读操作，迟早会工作完毕
		//因此，这个死循环，总能等到退出的时候，并不是死循环
		TonyXiaoMinSleep();
	}
}

void MRSWLock_DisableWrite(STonyXiaoMultiReadSingleWriteLock* pLock) {
	MUTEXLOCK(&(pLock->m_Lock));
	{
		pLock->m_bWriteFlag = false;
	}
	MUTEXUNLOCK(&(pLock->m_Lock));
}

//进入读函数，返回当前的读计数器值
int MRSWLock_AddRead(STonyXiaoMultiReadSingleWriteLock* pLock) {
	while (1) {
		//这里是死循环等待，不过，即使是其他线程在进行写操作，
		//操作总会完成，因此，总有机会碰到写标志为“假”，最终跳出循环
		MUTEXLOCK(&(pLock->m_Lock)); //加内部锁，进入锁域
		{
			if (!pLock->m_bWriteFlag) //检测写标志是否为“假”
			{
				//如果为“假”，表示可以开始读
				//此时，一定要先累加，再释放内部锁，避免由于空隙，
				//导致别的线程错误切入
				pLock->m_nReadCount++;
				MUTEXUNLOCK(&(pLock->m_Lock));
				//返回读计数器的值，
				//注意：这个值可能不一定是刚刚累加的值
				//由于内部锁已经解除，别的读线程完全可能切进来
				//将这个值增加好几次
				return MRSWLock_GetRead(pLock); //这是本函数唯一跳出点
			}
		} //如果写标志为“真”只能循环等待
		MUTEXUNLOCK(&(pLock->m_Lock));
		//使用特殊睡眠，后文有解释
		TonyXiaoMinSleep();
	}
}

//返回读计数器变化后的结果
int MRSWLock_DecRead(STonyXiaoMultiReadSingleWriteLock* pLock) {
	int nRet = 0;
	MUTEXLOCK(&(pLock->m_Lock));
	{
		//这是一种习惯性保护，递减计算时，如果最小值是0，总是加个判断
		if (0 < (pLock->m_nReadCount))
			pLock->m_nReadCount--;
		//注意，这里是直接获得读计数器的值，看起来，比进入读要准确一点
		nRet = pLock->m_nReadCount;
	}
	MUTEXUNLOCK(&(pLock->m_Lock));
	return nRet;
}

void MRSWLock_Read2Write(STonyXiaoMultiReadSingleWriteLock* pLock) {
	while (1) //死循环，和进入写中的死循环一个道理
	{
		MUTEXLOCK(&(pLock->m_Lock));
		{
			if (!pLock->m_bWriteFlag) {
				//注意这里，一旦检测到可以抢夺写权利
				//先把写标志置为“真”
				pLock->m_bWriteFlag = true;
				//切记，由于是读转写，以前进入读的时候，已经把计数器+1
				//这里一定要-1，否则会导致计数器永远不为0，系统挂死
				if (0 < (pLock->m_nReadCount))
					pLock->m_nReadCount--;
				//如前，所有状态设置完成，解除内部锁，跳到下一步
				MUTEXUNLOCK(&(pLock->m_Lock));
				goto MRSWLock_Read2Write_Wait_Read_Clean;
			}
		} //解除内部锁等待，前文已经说明
		MUTEXUNLOCK(&(pLock->m_Lock));
		TonyXiaoMinSleep();
	} //此处开始等待所有的读退出，同“进入写”的逻辑
	MRSWLock_Read2Write_Wait_Read_Clean: while (MRSWLock_GetRead(pLock)) {
		TonyXiaoMinSleep();
	}
}
/////////////////////////////////////////////////////////////////////////
//这里是实施部分，请注意各个函数内的锁调用，与前文不同
CMRSWint::CMRSWint() //构造函数初始化变量
{
	m_Lock.EnableWrite();
	m_nValue = 0;
	m_Lock.DisableWrite();
}
int CMRSWint::Get(void) //得到变量的值
		{
	int nRet = 0;
	m_Lock.AddRead(); //请关注这里，这是读方式，即这个动作可以并发
	{
		nRet = m_nValue;
	}
	m_Lock.DecRead();
	return nRet;
}
int CMRSWint::Set(int nValue) //设置变量的值
		{
	int nRet = 0;
	m_Lock.EnableWrite(); //注意，这里是写方式，表示是串行的
	{
		m_nValue = nValue;
		nRet = m_nValue;
	}
	m_Lock.DisableWrite();
	return nRet;
}
int CMRSWint::Add(int nValue) //加法运算
		{
	int nRet = 0;
	m_Lock.EnableWrite(); //写方式，串行
	{
		m_nValue += nValue;
		nRet = m_nValue;
	}
	m_Lock.DisableWrite();
	return nRet;
}
int CMRSWint::Dec(int nValue) //减法运算
		{
	int nRet = 0;
	m_Lock.EnableWrite(); //写方式，串行
	{
		m_nValue -= nValue;
		nRet = m_nValue;
	}
	m_Lock.DisableWrite();
	return nRet;
}
