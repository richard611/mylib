#ifndef __UTILS_H_
#define __UTILS_H_

//获得非零值
inline int _GetNot0(void) {
	int nRet = rand();
	if (!nRet)
		nRet++; //如果获得的随机数本身为0，返回1
	return nRet;
}

//获得0
inline int _Get0(void) {
	int nRet = rand();
	return nRet ^ nRet; //以异或方式求的0
}

//获得给定区间内的随机数
inline int GetRandomBetween(int nBegin, int nEnd) {
	int n = _GetNot0(); //获得一个随机数
	int nBetween = 0;
	if (0 > nBegin)
		nBegin = -nBegin; //防御性设计防止Begin 为负值
	if (0 > nEnd)
		nEnd = -nEnd; //防御性设计防止End 为负值
	if (nBegin > nEnd) //调整Begin 和End 的顺序，保证End>Begin
			{
		nBetween = nEnd;
		nEnd = nBegin;
		nBegin = nBetween;
	} else if (nBegin == nEnd) //如果给定的Beggin 和End 相等，即范围为0
		nEnd = nBegin + 10; //强行定义范围区间为10
	nBetween = nEnd - nBegin; //求的区间
	n = n % nBetween; //通过求余运算限幅
	n += nBegin; //与Begin 累加，确保结果落于Begin 和End 之间
	return n;
}

#define TimeSetNow(t) time(&t) //设置给定的t 为当前时间
//判断是否时间到了
// tLast：最后一次更新时间
//设定的最大时间
inline bool TimeIsUp(time_t tLast, long lMax) {
	time_t tNow;
	TimeSetNow(tNow);
	long lDeltaT = (long) tNow - (long) tLast;
	if (lMax <= lDeltaT)
		return true;
	else
		return false;
}
#endif
