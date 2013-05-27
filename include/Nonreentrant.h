#ifndef __NONREENTRANT_H_
#define __NONREENTRANT_H_

#include "MutexLock.h"

//不可重入锁类声明
class CNonReentrant {
public:
	CNonReentrant();
	~CNonReentrant() {
	} //析构函数不做任何事
public:
//设置为真的时候
// 如果没有设置进入标志，设置，并返回真
// 如果已经设置进入标志，不设置，并返回假
//设置为假的时候，
// 总是成功并返回真
	bool Set(bool bRunFlag);
private:
	CMutexLock m_Lock; //锁
	bool m_bAlreadRunFlag; //内部的变量值
};


#endif
