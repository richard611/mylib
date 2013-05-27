#include "Statistics.h"
#include "std.h"
//获得ΔX
unsigned long SCountSubGetX(SCountSub& CountSub) {
	return CountSub.m_lXEnd - CountSub.m_lXBegin;
} //设置初始值
void SCountSubSetBegin(SCountSub& CountSub, unsigned long n) {
	CountSub.m_lXBegin = n;
} //设置结束值
unsigned long SCountSubSetEnd(SCountSub& CountSub, unsigned long n) {
	CountSub.m_lXEnd = n;
	return SCountSubGetX(CountSub);
}

//初始化
void SCountReset(SCount& Count, unsigned long n) {
	Count.m_Sum = n;
	Count.m_Sub.m_lXBegin = 0;
	Count.m_Sub.m_lXEnd = 0;
} //计算统计平均值
unsigned long SCountSum(SCount& Count) {
	unsigned long X = SCountSubGetX(Count.m_Sub);
	if (0 == Count.m_Sum)
		Count.m_Sum = X; //初值如果为0,则直接赋值,避免误差
	else
		Count.m_Sum = (Count.m_Sum + X) / 2; //计算统计平均值
	return Count.m_Sum;
} //返回Sum 值
unsigned long SCountGetSum(SCount& Count) {
	return Count.m_Sum;
}
//返回当前的ΔX
unsigned long SCountGetX(SCount& Count) {
	return SCountSubGetX(Count.m_Sub);
}
//设置开始
void SCountSetBegin(SCount& Count, unsigned long n) {
	SCountSubSetBegin(Count.m_Sub, n);
}
//设置结束值,单纯更新,可以多次刷新
unsigned long SCountSetEnd(SCount& Count, unsigned long n) {
	return SCountSubSetEnd(Count.m_Sub, n);
}

void CDeltaTime::Reset(void) {
	m_CountSub.SetEnd(m_CountSub.SetBegin((unsigned long) time(null)));
}
//触发开始时间计数
void CDeltaTime::TouchBegin(void) {
	m_CountSub.SetBegin((unsigned long) time(null));
}
//触发结束时间技术
void CDeltaTime::TouchEnd(void) {
	m_CountSub.SetEnd((unsigned long) time(null));
}
//利用上文的Push 功能，实现循环统计。即Now->End->Begin
void CDeltaTime::Touch(void) {
	m_CountSub.Push((unsigned long) time(null));
}
