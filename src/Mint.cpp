#include "std.h"
#include "MutexLock.h"
#include "Mint.h"


//这里是.cpp 文件中的实现
int MvarInit(MINT& mValue, int nValue) {
	MUTEXINIT(&mValue.m_MyLock); //初始化函数主要就是初始化锁变量
	mValue.m_nValue = nValue; //同时赋初值
	return nValue;
}
void MvarDestroy(MINT& mValue) {
	MUTEXLOCK(&mValue.m_MyLock);
	MUTEXUNLOCK(&mValue.m_MyLock);
//还记得上面这个技巧吗？一次看似无意义的加锁和解锁行为
//使摧毁函数可以等待其他线程中没有完成的访问，最大限度保证安全
	MUTEXDESTROY(&mValue.m_MyLock); //摧毁锁变量
}
int MvarSet(MINT& mValue, int nValue) {
	MUTEXLOCK(&mValue.m_MyLock);
	mValue.m_nValue = nValue; //锁保护内的赋值
	MUTEXUNLOCK(&mValue.m_MyLock);
	return nValue;
}
int MvarGet(MINT& mValue) {
	int nValue;
	MUTEXLOCK(&mValue.m_MyLock);
	nValue = mValue.m_nValue; //锁保护内的取值
	MUTEXUNLOCK(&mValue.m_MyLock);
	return nValue;
}
int MvarADD(MINT& mValue, int nValue) {
	int nRet;
	MUTEXLOCK(&mValue.m_MyLock);
	mValue.m_nValue += nValue; //锁保护内的累加动作
	nRet = mValue.m_nValue;
	MUTEXUNLOCK(&mValue.m_MyLock);
	return nRet;
}
int MvarDEC(MINT& mValue, int nValue) {
	int nRet;
	MUTEXLOCK(&mValue.m_MyLock);
	mValue.m_nValue -= nValue; //锁保护内的减法计算
	nRet = mValue.m_nValue;
	MUTEXUNLOCK(&mValue.m_MyLock);
	return nRet;
}
