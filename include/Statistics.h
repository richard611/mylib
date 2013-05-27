#ifndef __STATISITCS_H_
#define __STATISITCS_H_
///////////////////////////////////
typedef struct _COUNT_SUB_ {
	unsigned long m_lXBegin;
	unsigned long m_lXEnd;
} SCountSub;
///////////////////////////////////
//基本统计平均值模块
typedef struct _COUNT_ {
	SCountSub m_Sub; //ΔX 计算
	unsigned long m_Sum; //统计平均值
} SCount;

////////////////////////////////////////////////////////////////////////
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" { //纯C 输出开始
#endif
//获得ΔX
extern unsigned long SCountSubGetX(SCountSub& CountSub);
//设置初始值
extern void SCountSubSetBegin(SCountSub& CountSub, unsigned long n) ;
//设置结束值
extern unsigned long SCountSubSetEnd(SCountSub& CountSub, unsigned long n) ;

//初始化
extern void SCountReset(SCount& Count, unsigned long n) ;
//计算统计平均值
extern unsigned long SCountSum(SCount& Count);
//返回Sum 值
extern unsigned long SCountGetSum(SCount& Count) ;

//返回当前的ΔX
extern unsigned long SCountGetX(SCount& Count) ;
//设置开始
extern void SCountSetBegin(SCount& Count, unsigned long n) ;
//设置结束值,单纯更新,可以多次刷新
extern unsigned long SCountSetEnd(SCount& Count, unsigned long n) ;

#if defined(__cplusplus) || defined(c_plusplus)
} //纯C 输出结束
#endif /*defined(__cplusplus) || defined(c_plusplus)*/

/////////////////////////////////////////////////////////////////////////
//△计算模块类
class CCountSub {
public:
	CCountSub() {
		m_CountSub.m_lXBegin = 0;
		m_CountSub.m_lXEnd = 0;
	}
	~CCountSub() {
	}
public:
	//设置初始值
	unsigned long SetBegin(unsigned long n) {
		return m_CountSub.m_lXBegin = n;
	}
	//设置结束值
	unsigned long SetEnd(unsigned long n) {
		return m_CountSub.m_lXEnd = n;
	}
	//获得初始值
	unsigned long GetBegin(void) {
		return m_CountSub.m_lXBegin;
	}
	//获得结束值
	unsigned long GetEnd(void) {
		return m_CountSub.m_lXEnd;
	}
	//获得△
	unsigned long GetX(void) {
		return m_CountSub.m_lXEnd - m_CountSub.m_lXBegin;
	}
	//把结束值赋给初始值，相当于△归零
	void E2B(void) {
		m_CountSub.m_lXBegin = m_CountSub.m_lXEnd;
	}
	//把结束值赋给初始值，然后给结束值赋新值，这在某些循环统计的场合非常有用
	void Push(unsigned long ulNew) {
		E2B();
		m_CountSub.m_lXEnd = ulNew;
	}
	//与前文共用的数据描述结构体
	SCountSub m_CountSub;
};

/////////////////////////////////////////////////////////////////////////////
//平均值类
//
class CDeltaTime {
public:
	CDeltaTime() {
		TouchBegin();
	}
	~CDeltaTime() {
	}
public:
	//Reset 功能，初始化统计时钟，且Begin=End
	void Reset(void) ;
	//触发开始时间计数
	void TouchBegin(void) ;
	//触发结束时间技术
	void TouchEnd(void) ;
	//利用上文的Push 功能，实现循环统计。即Now->End->Begin
	void Touch(void) ;
	//获得△
	unsigned long GetDeltaT(void) {
		return m_CountSub.GetX();
	}
	//平均值计算获得每秒钟的操作数，用户以累加器统计总的操作数，传递进来，
	//本函数自动根据内部的△计算平均值，除法计算提供double 的精度，并规避除0 错误
	double GetOperationsPreSecond(unsigned long ulOperationCount) {
		double dRet = 0.0;
		double dCount = (double) ulOperationCount;
		unsigned long ulDeltaSecond = GetDeltaT();
		double dSeconds = (double) ulDeltaSecond;
		if (0 == ulDeltaSecond)
			return dRet;
		return dCount / dSeconds;
	} //以前文模块描述的基本统计模块
	CCountSub m_CountSub;
};
////////////////////////////////////////////////////////////////////////////////////
////带锁的时间平均值统计模块，所有函数功能同上，本层仅仅提供锁保护
//class CDeltaTimeVSLock {
//public:
//	CDeltaTimeVSLock() {
//	}
//	~CDeltaTimeVSLock() {
//	}
//public:
//	void Reset(void) { //Reset 为写动作，不能并发
//		m_Lock.EnableWrite();
//		m_nDeltaT.Reset();
//		m_Lock.DisableWrite();
//	}
//	void TouchBegin(void) { //TouchBegin 为写动作，不能并发
//		m_Lock.EnableWrite();
//		m_nDeltaT.TouchBegin();
//		m_Lock.DisableWrite();
//	}
//	void TouchEnd(void) { //TouchEnd 为写动作，不能并发
//		m_Lock.EnableWrite();
//		m_nDeltaT.TouchEnd();
//		m_Lock.DisableWrite();
//	}
//	unsigned long GetDeltaT(void) { //GetDeltaT 为读动作，可以并发读
//		unsigned long nRet = 0;
//		m_Lock.AddRead();
//		nRet = m_nDeltaT.GetDeltaT();
//		m_Lock.DecRead();
//		return nRet;
//	}
//	double GetOperationsPreSecond(unsigned long ulOperationCount) { //GetOterationPerSecond 为读动作，可以并发读
//		double dRet = 0.0;
//		m_Lock.AddRead();
//		dRet = m_nDeltaT.GetOperationsPreSecond(ulOperationCount);
//		m_Lock.DecRead();
//		return dRet;
//	}
//private:
//	CDeltaTime m_nDeltaT; //基本统计因子
//	CMultiReadSingleWriteLock m_Lock; //单写多读锁对象
//};


//////////////////////////////////////////////////////////////////////////////
class CCount {
public:
	CCount() {
		SCountReset(0);
	}
	~CCount() {
	}
public:
	void SCountReset(unsigned long n) {
		m_Count.m_Sum = n;
		m_Count.m_Sub.m_lXBegin = 0;
		m_Count.m_Sub.m_lXEnd = 0;
	}
	unsigned long SCountSum(void) {
		unsigned long X = SCountSubGetX(m_Count.m_Sub);
		if (0 == m_Count.m_Sum)
			m_Count.m_Sum = X; //初值如果为0,则直接赋值,避免误差
		else
			m_Count.m_Sum = (m_Count.m_Sum + X) / 2; //计算统计平均值
		return m_Count.m_Sum;
	}
	unsigned long SCountSetSum(unsigned long n) {
		m_Count.m_Sum = n;
		return n;
	}
	unsigned long SCountGetSum(void) {
		return m_Count.m_Sum;
	}
	unsigned long SCountGetX(void) {
		return SCountSubGetX(m_Count.m_Sub);
	}
	void SCountSetBegin(unsigned long n) {
		SCountSubSetBegin(m_Count.m_Sub, n);
	}
	unsigned long SCountSetEnd(unsigned long n) {
		return SCountSubSetEnd(m_Count.m_Sub, n);
	}
	SCount m_Count;
};
#endif