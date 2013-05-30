#include "std.h"
#include "Debug.h"
#include "TonyLowDebug.h"
#include "MemoryManager.h"
#include "Buffer.h"

/////////////////////////////////////////////////////////////////////////
//******************************动态buffer类****************************//
/////////////////////////////////////////////////////////////////////////
#ifdef __DYNAMIC_BUFF_
CTonyBuffer::CTonyBuffer(CTonyMemoryPoolWithLock* pMemPool) {
	m_pMemPool = pMemPool; //保存内存池指针
	m_pData = null;//注意此处，动态内存块指针并未分配，先保持空
	m_nDataLength = 0;//长度也保持为0
}
CTonyBuffer::~CTonyBuffer() {
	SetSize(0); //SetSize 为0，表示清除动态内存块，下文有介绍
}
//由于动态内存申请，可能会失败，因此返回bool 量，成功为真，失败返回假
bool CTonyBuffer::SetSize(int nSize)//给定新的大小
{
	if (!m_pMemPool)
	return false; //防御性设计，如果内存池指针为空，无法工作
	m_nDataLength = nSize;//先给缓冲区长度赋值
	if (0 == m_nDataLength)//注意析构函数中的SetSize(0)，此处处理
	{ //如果设置大小为0，表示释放缓冲区
		if (m_pData) {
			m_pMemPool->Free(m_pData); //内存池释放
			m_pData = null;//释放后立即赋初值null，避免下文误用
		}
		return true; //这也是操作成功，返回真
	} //此处开始 ，新设置的缓冲期长 度，一定不为0
	if (!m_pData)//如果原指针为空，表示没有数据，需要Malloc
	{
//请注意这里对内存池Malloc 函数的调用，这也算是内存池的应用实例
//内存池申请的指针都是void*，这符合C 语言malloc 的规范，
//而本类的指针都是字符串型，因此需要做强制指针类型转换
		m_pData = (char*) m_pMemPool->Malloc(m_nDataLength,//申请的内存块长度
				"CTonyBuffer::m_pData");//注意，这里特别声明申请者身份
//一旦发生内存泄漏，本对象退出时忘了释放
//内存池的报警机制即会激活，打印这个字符串
		if (!m_pData)//请注意，这里二次判断，是判断申请是否成功
		{
			m_nDataLength = 0; //没有成功，则把长度赋为0
			return false;//申请失败，返回假
		} else
		return true; //成功就返回真
	} else //这是原有m_pData 不为空，就是已经有一个内存区的情况
	{
//使用ReMalloc 函数做内存区域再调整，默认，拷贝原有内容
//请注意，这里是内存池ReMalloc 功能的实例，将原有指针传入
//经过ReMalloc 修饰后，返回新的指针，注意强制指针类型转换
		m_pData = (char*) m_pMemPool->ReMalloc(m_pData, m_nDataLength);
		if (!m_pData) {
			m_nDataLength = 0; //申请失败，返回假
			return false;
		} else
		return true; //成功返回真
	}
}
//在前面插入空白，同上，动态内存申请可能失败，所以返回bool 量
bool CTonyBuffer::InsertSpace2Head(int nAddBytes) {
	bool bRet = false; //预设返回值
	int nNewSize = 0;//新的空间大小变量
	char* pBuffer = null;//先定义一个新的二进制缓冲区指针
	if (!m_pMemPool)//防御性设计，防止内存池为空
	goto CTonyBuffer_InsertSpace2Head_End_Process;
	nNewSize = m_nDataLength + nAddBytes;//求得新的长度
//请注意这里，申请一个临时缓冲区，其长度等于原长度+增加的长度
	pBuffer = (char*) m_pMemPool->Malloc(nNewSize,
			"CTonyBuffer::InsertSpace2Head():pBuffer");//请注意这段说明文字
	if (!pBuffer)//缓冲区申请失败，跳转返回假
	goto CTonyBuffer_InsertSpace2Head_End_Process;
//此处为防御性设计，如果原有缓冲区为空，则不做后续的拷贝动作
	if ((m_pData) && (m_nDataLength)) { //这是将原有缓冲区内容拷贝到临时缓冲区后半部分，与新增加字节构成一个整体
		memcpy(pBuffer + nAddBytes, m_pData, m_nDataLength);
	} //当所有准备工作完成，调用本对象的二进制拷贝函数，将临时缓冲区内容拷贝回本对象
//二进制拷贝函数，后文有介绍，当然，其拷贝成功与否，也作为本函数的返回值
	bRet = BinCopyFrom(pBuffer, nNewSize);
	CTonyBuffer_InsertSpace2Head_End_Process:
//不管是否成功拷贝，释放临时缓冲区
	if (pBuffer) {
		m_pMemPool->Free(pBuffer);
		pBuffer = null;
	}
	return bRet; //返回操作结果
}
//在后面插入空白
bool CTonyBuffer::AddSpace2Tail(int nAddBytes) {
	return SetSize(m_nDataLength + nAddBytes);
}
//从前面剪切掉一段数据
void CTonyBuffer::CutHead(int nBytes) {
//防御性设计，如果给出的剪切空间大于原有缓冲区，则直接清空。
	if (m_nDataLength <= nBytes)
	SetSize(0);
	else { //这是从后向前Move，因此，直接调用memecpy 完成。
		memcpy(m_pData, m_pData + nBytes, m_nDataLength - nBytes);
//大家请注意，这里笔者没有再SetSize，由于内存池的原理，
//ReMalloc 一个比较小的空间，一般都是直接返回原指针，
//因此，此处也不多此一举了，直接就把空间长度修改为较小的长度即可
		m_nDataLength -= nBytes;
	}
}
//从后面剪切掉一段数据
void CTonyBuffer::CutTail(int nBytes) {
//防御性设计，剪切太多，直接清空
	if (m_nDataLength <= nBytes)
	SetSize(0);
//同上，减小直接修改长度参数
	else
	m_nDataLength -= nBytes;
}
//返回拷贝的字节数，拷贝失败，返回0
int CTonyBuffer::BinCopyTo(char* szBuffer,//调用者给出缓冲区指针
		int nBufferSize)//调用者给出缓冲区大小
{ //防御性设计
	if (!m_pData)
	return 0;//如果内部无数据，返回0
	if (!m_nDataLength)
	return 0;
	if (nBufferSize < m_nDataLength)
	return 0;//如果给定缓冲区小于本数据缓冲区，返回0
	memcpy(szBuffer, m_pData, m_nDataLength);//执行拷贝动作
	return m_nDataLength;//返回拷贝的字节长度
}
//拷贝一个二进制缓冲区的数据
int CTonyBuffer::BinCopyFrom(char* szData, int nDataLength) {
//防御性设计，如果给定的参数非法，清空本地数据，返回0
	if ((!szData) || (0 >= nDataLength)) {
		SetSize(0);
		return 0;
	}
	if (!SetSize(nDataLength))
	return 0; //重新设置
	memcpy(m_pData, szData, m_nDataLength);
	return m_nDataLength;
} //拷贝同类，另外一个对象的数据
int CTonyBuffer::BinCopyFrom(CTonyBuffer* pBuffer) { //这里调用上一函数完成功能，注意工具类的用法，直接访问目标内部数据区
	return BinCopyFrom(pBuffer->m_pData, pBuffer->m_nDataLength);
}
//拷贝一个整数到本对象，执行网络字节序，破坏原有数据
bool CTonyBuffer::SetInt(int n) {
	int nSave = htonl(n); //以临时变量求得给定整数的网络字节序
//拷贝到本地缓冲区，带SetSize
	return BinCopyFrom((char*) &nSave, sizeof(int));
} //以整数本地字节序求得缓冲区最开始4Bytes 构成的整数的值，求值失败，返回0
int CTonyBuffer::GetInt(void) {
//防御性设计，如果本对象没有存储数据，或者存储的数据不到一个整数位宽4Bytes，返回0
	if (!m_pData)
	return 0;
	if (!sizeof(int) > m_nDataLength)
	return 0;
	return ntohl(*(int*) m_pData);//以头4Bytes 数据，求得本地字节序返回
}
bool CTonyBuffer::SetShort(short n) {
	short sSave = htons(n);
	return BinCopyFrom((char*) &sSave, sizeof(short)); //拷贝的字节数变成短整型位宽
}
short CTonyBuffer::GetShort(void) {
	if (!m_pData)
	return 0;
	if (sizeof(short) > m_nDataLength)
	return 0; //注意，此处的范围变成短整型的位宽
	return ntohs(*(short*) m_pData);
}
bool CTonyBuffer::SetChar(char n) {
	return BinCopyFrom(&n, sizeof(char)); //位宽1Byte
}
char CTonyBuffer::GetChar(void) {
	if (!m_pData)
	return 0;
	if (sizeof(char) > m_nDataLength)
	return 0; //位宽1Byte
	return *(char*) m_pData;
}
int CTonyBuffer::AddData(char* szData, int nDataLength) {
	if ((!m_pData) || (0 >= m_nDataLength)) { //防御性设计，如果原有数据为空，则直接执行拷贝动作
		return BinCopyFrom(szData, nDataLength);
	}
	int nOldSize = m_nDataLength; //保留原有大小
	int nNewSize = m_nDataLength + nDataLength;//求得新的大小
	if (!SetSize(nNewSize))
	return 0;//SetSize，其逻辑保留原有数据
//注意，失败返回0
	memcpy(m_pData + nOldSize, szData, nDataLength);//拷贝新数据到原有数据末尾
	return m_nDataLength;//返回新的大小
}
int CTonyBuffer::InsertData2Head(char* szData, int nDataLength) {
	if ((!m_pData) || (0 >= m_nDataLength)) { //防御性设计，如果原有数据为空，则直接执行拷贝动作
		return BinCopyFrom(szData, nDataLength);
	} //先在前插入足够空区，保证后续拷贝动作能成功。
//根据InsertSpace2Head 逻辑，原有数据可以获得保留
	if (!InsertSpace2Head(nDataLength))
	return 0;
//拷贝新数据到开始，注意拷贝的长度是新数据的长度，因此，不会破坏后续的原有数据
	memcpy(m_pData, szData, nDataLength);
	return m_nDataLength;//返回新的大小
}
int CTonyBuffer::StrCopyFrom(char* szString) {
	int n = strlen(szString); //先求出目标字符串的长度
	n++;//长度+1，包括’\0’的位置
	return BinCopyFrom(szString, n);//调用二进制拷贝函数完成动作
}
//变参设计，返回打印的字符串总字节数(包含’\0’的位宽)
int CTonyBuffer::Printf(char* szFormat, ...) {
//这一段在很多变参函数中都出现过，此处不再赘述
	char szBuf[TONY_BUFFER_STRING_MAX];
	int nListCount = 0;
	va_list pArgList;
	va_start(pArgList, szFormat);
	nListCount += Linux_Win_vsnprintf(szBuf + nListCount,
			TONY_BUFFER_STRING_MAX - nListCount, szFormat, pArgList);
	va_end(pArgList);
	if (nListCount > (TONY_BUFFER_STRING_MAX - 1))
	nListCount = TONY_BUFFER_STRING_MAX - 1;
	*(szBuf + nListCount) = '\0';
//最后调用StrCopyFrom，将缓冲区内容拷贝到本对象
	return StrCopyFrom(szBuf);
}
//二进制比较
int CTonyBuffer::memcmp(char* szData, int nDataLength) { //防御性设计
	if (!m_pData)
	return -1;
	if (!m_nDataLength)
	return -1;
	if (!szData)
	return -1;
	if (m_nDataLength < nDataLength)
	return -1;
//使用C 标准库的memcmp 完成功能
	int nRet = ::memcmp(m_pData, szData, nDataLength);
	return nRet;
} //文本型比较
int CTonyBuffer::strcmp(char* szString) { //防御性设计
	if (!m_pData)
	return -1;
	if (!m_nDataLength)
	return -1;
	if (!szString)
	return -1;
//使用C 标准库的strcmp 完成功能
	int nRet = ::strcmp(m_pData, szString);
	return nRet;
}
/////////////////////////////////////////////////////////////////////////
//******************************静态buffer类****************************//
/////////////////////////////////////////////////////////////////////////
#else
CTonyBuffer::CTonyBuffer(CTonyBaseLibrary* pTonyBaseLib) {
	m_pMemPool = pTonyBaseLib->m_pMemPool; //保留内存池对象
	m_nDataLength = 0; //数据长度为0
}
CTonyBuffer::CTonyBuffer(CTonyMemoryPoolWithLock* pMemPool) {
	m_pMemPool = pMemPool; //保留内存池对象
	m_nDataLength = 0; //数据长度为0
}
bool CTonyBuffer::IHaveData(void) //已有数据标志
		{ //由于使用静态数组，m_pData 永远有意义，因此，此处仅判断m_nDataLength 的值
	if (0 >= m_nDataLength)
		return false;
	return true;
}
bool CTonyBuffer::SetSize(int nSize) {
	if (TONY_SAFE_BUFFER_MAX_SIZE < nSize) { //如果超界，报警，返回假，即告知调用者分配失败
		m_pMemPool->m_pDebug->Debug2File(
				"CTonyBuffer::SetSize(): ERROR! nSize=%d\n", nSize);
		return false;
	}
	m_nDataLength = nSize; //数据长度置为设置尺寸，
//这其实还是把m_nDataLength 作为缓冲区大小提示
	return true;
}
bool CTonyBuffer::InsertSpace2Head(int nAddBytes) {
	if (0 >= m_nDataLength) { //如果没有原始数据，则视为设置缓冲区大小
		m_nDataLength = nAddBytes;
		return true;
	} //这个计算相对复杂，首先缓冲区最大尺寸恒定，因此，它与原有数据的差值，是新插入空区的
	  //最大可能值，因此，此处必须做判断
	if (nAddBytes > (TONY_SAFE_BUFFER_MAX_SIZE - m_nDataLength)) { //条件不满足，则报警返回
		m_pMemPool->m_pDebug->Debug2File(
				"CTonyBuffer::InsertSpace2Head(): ERROR! nAddBytes=%d, m_nDataLength=%d, too big!\n",
				nAddBytes, m_nDataLength);
		return false;
	}
	{
		//注意此处的大括号，是限定变量szBuffer 的作用范围
		char szBuffer[TONY_SAFE_BUFFER_MAX_SIZE];
		memset(szBuffer, '\0', TONY_SAFE_BUFFER_MAX_SIZE);
		//第一次，把原有数据拷贝到新缓冲区，已经偏移了nAddBytes 的位置
		memcpy(szBuffer + nAddBytes, m_pData, m_nDataLength);
		m_nDataLength += nAddBytes;
		//第二次，将新缓冲区有效数据拷贝回本对象缓冲区
		memcpy(m_pData, szBuffer, m_nDataLength);
		//之所以这么复杂的拷贝，主要就是为了规避前文所述的“从前拷贝”和“从后拷贝”问题
	}
	return true;
}
bool CTonyBuffer::AddSpace2Tail(int nAddBytes) //在后面插入空白
		{
	if (0 >= m_nDataLength) { //如果没有原始数据，则视为设置缓冲区大小
		m_nDataLength = nAddBytes;
		return true;
	} //判断新设置的尺寸大小是否合适 
	if (nAddBytes > (TONY_SAFE_BUFFER_MAX_SIZE - m_nDataLength)) {
		m_pMemPool->m_pDebug->Debug2File(
				"CTonyBuffer::AddSpace2Tail(): ERROR! nAddBytes=%d, m_nDataLength=%d, too big!\n",
				nAddBytes, m_nDataLength);
		return false;
	}
	m_nDataLength += nAddBytes; //后面追加比较简单，修改m_nDataLength 的值即可
	return true;
}
void CTonyBuffer::CutHead(int nBytes) //从前面剪切掉一段数据
		{
	if (0 >= m_nDataLength) { //没有原始数据，剪切无意义，报警，宣告失败
		m_pMemPool->m_pDebug->Debug2File(
				"CTonyBuffer::CutHead(): ERROR! m_nDataLength=%d, too small!\n",
				m_nDataLength);
		m_nDataLength = 0;
		return;
	}
	if (nBytes > m_nDataLength) { //如果剪切的数据长度大于原始数据长度，报警
		m_pMemPool->m_pDebug->Debug2File(
				"CTonyBuffer::CutHead(): ERROR! m_nDataLength=%d, nBytes=%d, too small!\n",
				m_nDataLength, nBytes);
		m_nDataLength = 0;
		return;
	}
	m_nDataLength -= nBytes; //求出新的数据长度
//后面数据向前拷贝，“挤出”原有数据
	memcpy(m_pData, m_pData + nBytes, m_nDataLength);
	return;
}
void CTonyBuffer::CutTail(int nBytes) //从后面剪切掉一段数据
		{
	if (0 >= m_nDataLength) { //没有原始数据，剪切无意义，报警，宣告失败
		m_pMemPool->m_pDebug->Debug2File(
				"CTonyBuffer::CutTail(): ERROR! m_nDataLength=%d, too small!\n",
				m_nDataLength);
		m_nDataLength = 0;
		return;
	}
	if (nBytes > m_nDataLength) { //如果剪切的数据长度大于原始数据长度，报警
		m_pMemPool->m_pDebug->Debug2File(
				"CTonyBuffer::CutTail(): ERROR! m_nDataLength=%d, nBytes=%d, too small!\n",
				m_nDataLength, nBytes);
		m_nDataLength = 0;
		return;
	}
	m_nDataLength -= nBytes; //缩短长度就是剪切尾部
	return;
}
int CTonyBuffer::BinCopyTo(char* szBuffer, int nBufferSize) {
//防御性设计，条件不满足则报警，返回0，表示没有拷贝成功
	if (m_nDataLength > nBufferSize) {
		m_pMemPool->m_pDebug->Debug2File(
				"CTonyBuffer::BinCopyTo(): ERROR! nBufferSize=%d,m_nDataLength=%d\n",
				nBufferSize, m_nDataLength);
		return 0;
	}
	if (!szBuffer) {
		m_pMemPool->m_pDebug->Debug2File(
				"CTonyBuffer::BinCopyTo(): ERROR! szBuffer=null\n");
		return 0;
	} //执行真的拷贝逻辑
	memcpy(szBuffer, m_pData, m_nDataLength);
	return m_nDataLength; //返回拷贝的字节数
}
int CTonyBuffer::BinCopyFrom(char* szData, int nDataLength) {
//防御性设计
	if (TONY_SAFE_BUFFER_MAX_SIZE < nDataLength) {
		m_pMemPool->m_pDebug->Debug2File(
				"CTonyBuffer::BinCopyFrom(): ERROR! nDataLength=%d, too big!\n",
				nDataLength);
		return 0;
	}
	if (!szData) {
		m_pMemPool->m_pDebug->Debug2File(
				"CTonyBuffer::BinCopyTo(): ERROR! szData=null\n");
		return 0;
	}
	if (0 >= nDataLength) {
		m_pMemPool->m_pDebug->Debug2File(
				"CTonyBuffer::BinCopyTo(): ERROR! 0>=nDataLength\n");
		return 0;
	} //真实的拷贝动作
	memcpy(m_pData, szData, nDataLength);
	m_nDataLength = nDataLength;
	return m_nDataLength; //返回拷贝的字节数
}
int CTonyBuffer::BinCopyFrom(CTonyBuffer* pBuffer) { //拷贝另外一个buffer
	return BinCopyFrom(pBuffer->m_pData, pBuffer->m_nDataLength);
}
bool CTonyBuffer::SetInt(int n) //设置一个整数，网络格式
		{
	int nSave = htonl(n);
	return BinCopyFrom((char*) &nSave, sizeof(int));
}
int CTonyBuffer::GetInt(void) //获得一个整数，返回本地格式
		{
	if (0 >= m_nDataLength)
		return 0;
	int* pData = (int*) m_pData;
	int nRet = *pData;
	return ntohl(nRet);
}
bool CTonyBuffer::SetShort(short n) //设置一个短整数，网络格式
		{
	short sSave = htons(n);
	return BinCopyFrom((char*) &sSave, sizeof(short));
}
short CTonyBuffer::GetShort(void) //获得一个短整数，返回本地格式
		{
	if (0 >= m_nDataLength)
		return 0;
	short* pData = (short*) m_pData;
	short sRet = *pData;
	return ntohs(sRet);
}
bool CTonyBuffer::SetChar(char n) //设置一个字节
		{
	*m_pData = n;
	m_nDataLength = sizeof(char);
	return true;
}
char CTonyBuffer::GetChar(void) //得到m_pData 第一个字节的值
		{
	return *m_pData;
}
//二进制数据追加函数
//追加数据到最后，返回新的数据长度
int CTonyBuffer::AddData(char* szData, int nDataLength) {
	int nNewSize = m_nDataLength + nDataLength; //求得新的尺寸
	if (TONY_SAFE_BUFFER_MAX_SIZE < nNewSize) //防御性判断
			{
		m_pMemPool->m_pDebug->Debug2File(
				"CTonyBuffer::AddData(): ERROR! m_nDataLength=%d, nDataLength=%d, too big!\n",
				m_nDataLength, nDataLength);
		return 0;
	} //做真实的拷贝动作
	memcpy(m_pData + m_nDataLength, szData, nDataLength);
	m_nDataLength = nNewSize;
	return m_nDataLength;
}
//插入数据到最前面，返回新的数据长度
int CTonyBuffer::InsertData2Head(char* szData, int nDataLength) {
	if (!InsertSpace2Head(nDataLength)) //先试图插入空白到最前
			{
		m_pMemPool->m_pDebug->Debug2File(
				"CTonyBuffer::InsertData2Head(): ERROR! m_nDataLength=%d, nDataLength=%d, too big!\n",
				m_nDataLength, nDataLength);
		return 0;
	}
	memcpy(m_pData, szData, nDataLength); //成功则拷贝
	return m_nDataLength;
}
int CTonyBuffer::StrCopyFrom(char* szString) //拷贝一个字符串到内部
		{
	int nDataLength = strlen(szString) + 1; //求出字符串数据长度，
//+1 表示包含’\0’
	return BinCopyFrom(szString, nDataLength); //调用BinCopyFrom 完成拷贝
}
int CTonyBuffer::Printf(char* szFormat, ...) //变参打印构造一个字符串 
		{
	char szBuf[TONY_SAFE_BUFFER_MAX_SIZE]; //注意，最大长度为静态buffer
//的最大长度
	int nListCount = 0;
	va_list pArgList;
	va_start(pArgList, szFormat);
	nListCount += Linux_Win_vsnprintf(szBuf + nListCount,
			TONY_SAFE_BUFFER_MAX_SIZE - nListCount, szFormat, pArgList);
	va_end(pArgList);
	if (nListCount > (TONY_SAFE_BUFFER_MAX_SIZE - 1))
		nListCount = TONY_SAFE_BUFFER_MAX_SIZE - 1;
	*(szBuf + nListCount) = '\0';
//以上为变参处理段落，已经多处出现，此处不再赘述
	return StrCopyFrom(szBuf); //调用StrCopyFrom 完成拷贝
}
int CTonyBuffer::memcmp(char* szData, int nDataLength) //二进制比较
		{ //防御性设计
	if (0 >= m_nDataLength)
		return -1;
	if (!szData)
		return -1;
	if (m_nDataLength != nDataLength)
		return -1;
	int nRet = ::memcmp(m_pData, szData, nDataLength); //调用系统函数完成
	return nRet;
}
int CTonyBuffer::strcmp(char* szString) //字符串比较
		{ //防御性设计
	if (0 >= m_nDataLength)
		return -1;
	if (!szString)
		return -1;
	int nRet = ::strcmp(m_pData, szString); //调用系统函数完成
	return nRet;
}
#endif
/////////////////////////////////////////////////////////////////////////
//******************************pop buffer类***************************//
/////////////////////////////////////////////////////////////////////////
CTonyPopBuffer::CTonyPopBuffer(char* szBuffer, int nBufferSize,
		bool bInitFlag) {
	m_pHead = null; //初始化各种管件变量
	m_pBuffer = null;
	m_nBufferSize = 0;
	Set(szBuffer, nBufferSize); //调用后粘合的Set 函数，实现粘合
	if (bInitFlag)
		Clean(); //如果需要初始化，则清空整个队列
}
CTonyPopBuffer::~CTonyPopBuffer() {
}
bool CTonyPopBuffer::ICanWork(void) { //依次检测所有关键变量
	if (!m_pBuffer)
		return false;
	if (!m_nBufferSize)
		return false;
	if (!m_pHead)
		return false;
	return true;
}
void CTonyPopBuffer::Set(char* szBuffer, int nBufferSize) {
	m_pBuffer = szBuffer; //挂接缓冲区
	m_nBufferSize = nBufferSize;
	m_pHead = (STonyPopBufferHead*) m_pBuffer; //定义队列头指针
}
void CTonyPopBuffer::Clean(void) {
	if (m_pHead) //注意，此处已经开始使用m_pHead
	{
		m_pHead->m_nTokenCount = 0; //所有Token 总数置为0
//总消耗的字节数，置为队列头的长度。
//这表示，PopBuffer 最后输出的总字节数，是包含这个队列头长度的。
//同时也表示，即使PopBuffer 内部一个Token 都没有，其字节数也不为0
		m_pHead->m_nAllBytesCount = STonyPopBufferHeadSize;
	}
}
//格式化输出内部数据
void CTonyPopBuffer::PrintInside(void) {
	if (!ICanWork()) { //防御性设计
		TONY_XIAO_PRINTF(
				"CTonyPopBuffer::PrintInside(): \
ERROR! m_pBuffer=null\n");
		return;
	} //定义元素区开始的指针
	char* pTokenBegin = TONY_POP_BUFFER_FIRST_TOKEN_BEGIN(m_pBuffer);
//定义元素的头指针
	STonyPopBufferTokenHead* pTokenHead = (STonyPopBufferTokenHead*) pTokenBegin;
//利用pTokenHead，偏移计算本Token 的数据开始点
//请注意，这里传入的pTokenHead，就不是字符串型，如果前文函数型宏定义中
//没有做强制指针类型转换，此处已经出错。
	char* pTokenData = TONY_POP_BUFFER_TOKEN_DATA_BEGIN(pTokenBegin);
	int i = 0;
//预打印整个队列的信息，即元素个数，字节数。
	TONY_XIAO_PRINTF("CTonyPopBuffer::PrintInside(): Token: %d Bytes: %d\n",
			m_pHead->m_nTokenCount, m_pHead->m_nAllBytesCount);
	for (i = 0; i < m_pHead->m_nTokenCount; i++) {
//注意，由于队列中存储的数据，可能是文本型，也可能是二进制型
//笔者在此准备了两条打印语句，使用时，根据情况选择
		TONY_XIAO_PRINTF("[%d] - %s\n", //格式化输出文本
				pTokenHead->m_nDataLength, pTokenData);
//dbg_bin(pTokenData,pTokenHead->m_nDataLength); //格式化输出二进制
//开始迭代计算，根据本Token 长度，计算下一Token 起始点
		pTokenBegin += TONY_POP_BUFFER_TOKEN_LENGTH(pTokenHead->m_nDataLength);
//修订相关Token 头等参变量
		pTokenHead = (STonyPopBufferTokenHead*) pTokenBegin;
		pTokenData = TONY_POP_BUFFER_TOKEN_DATA_BEGIN(pTokenBegin);
	}
}
int CTonyPopBuffer::GetTokenCount(void) {
	if (!ICanWork())
		return 0; //防御性设计
	return m_pHead->m_nTokenCount; //返回元素个数
}
int CTonyPopBuffer::GetAllBytes(void) {
	if (!ICanWork())
		return 0; //防御性设计
	return m_pHead->m_nAllBytesCount; //返回所有占用的字节数
}
//检查剩余空间是否够存储给定的数据长度
bool CTonyPopBuffer::ICanSave(int nDataLength) {
	int nLeaveBytes = 0; //准备变量，求得剩余空间
	if (!ICanWork())
		return false; //防御性设计
//剩余空间=缓冲区总长度-AllBytes，我们知道，AllBytes 里面已经包含了队列头长度
//因此，这种计算是正确的。
	nLeaveBytes = m_nBufferSize - m_pHead->m_nAllBytesCount;
//判断语句，注意，进入的长度，需要利用函数型宏进行修饰。
//由于每个Token 有一个小的头，这个头的长度，需要叠加进来判断，否则就不准确
	if (TONY_POP_BUFFER_TOKEN_LENGTH(nDataLength) > (ULONG) nLeaveBytes)
		return false;
	else
		return true;
}
//针对普通缓冲区的AddLast
int CTonyPopBuffer::AddLast(char* szData, int nDataLength) {
	int nRet = 0; //准备返回值
	//防御性设计
	if (!szData)
		goto CTonyPopBuffer_AddLast_End_Process;
	if (0 >= nDataLength)
		goto CTonyPopBuffer_AddLast_End_Process;
	if (!ICanWork())
		goto CTonyPopBuffer_AddLast_End_Process;
	//如果剩余空间不够添加，则跳转返回0
	if (!ICanSave(nDataLength))
		goto CTonyPopBuffer_AddLast_End_Process;
	{ //请注意，这个大括号不是if 语句开始，上文if 语句如果成立，已经goto 跳转
	  //此处主要是为了限定变量的作用域，gcc 中，不允许在goto 语句之后声明变量
	  //此处的大括号，是为了重新开辟一个堆栈区，以便声明局部变量
		char* pTokenBegin = //利用AllBytes 求出队列最尾的偏移
				m_pBuffer + m_pHead->m_nAllBytesCount;
		STonyPopBufferTokenHead* pTokenHead = //强制指针类型转换为Token 头指针
				(STonyPopBufferTokenHead*) pTokenBegin;
		char* pTokenData = //求出Token 数据区的指针
				TONY_POP_BUFFER_TOKEN_DATA_BEGIN(pTokenBegin);
		//请注意具体的添加动作
		pTokenHead->m_nDataLength = nDataLength; //先给元素头中的长度赋值
		memcpy(pTokenData, szData, nDataLength); //memcpy 数据内容到缓冲区
		m_pHead->m_nTokenCount++; //元素个数+1
		//请注意，这里AllByutes 添加的是包括元素头的所有长度，而不仅仅是数据长度
		m_pHead->m_nAllBytesCount += //AllByutes 加上增加的长度
				TONY_POP_BUFFER_TOKEN_LENGTH(nDataLength);
		nRet = nDataLength; //但返回值纯数据长度
	}
	CTonyPopBuffer_AddLast_End_Process: //结束跳转点
	return nRet; //返回结果
} //针对Buffer 类的AddLast
int CTonyPopBuffer::AddLast(CTonyBuffer* pBuffer) {
	if (!pBuffer)
		return 0;	//防御性设计
	//调用上一函数，实现功能
	return AddLast(pBuffer->m_pData, pBuffer->m_nDataLength);
}
int CTonyPopBuffer::GetFirstTokenLength(void) {
	if (!ICanWork())
		return 0; //防御性设计
	char* pFirstTokenBegin = //利用宏计算第一个Token 起始点
			TONY_POP_BUFFER_FIRST_TOKEN_BEGIN(m_pBuffer);
	STonyPopBufferTokenHead* pFirstTokenHead = //获得Token 头指针
			(STonyPopBufferTokenHead*) pFirstTokenBegin;
	return pFirstTokenHead->m_nDataLength; //返回头中包含的数据长度
}
//以普通缓冲区方式获得第一个Token 数据，上层程序保证缓冲区足够大，并传入供检查
int CTonyPopBuffer::GetFirst(char* szBuffer, int nBufferSize) {
	int nRet = 0; //准备返回参数
//防御性设计
	if (!ICanWork())
		goto CTonyPopBuffer_GetFirst_End_Process;
//判定队列是否为空
	if (!GetTokenCount())
		goto CTonyPopBuffer_GetFirst_End_Process;
//判断给定的参数区是否合法
	if (GetFirstTokenLength() > nBufferSize)
		goto CTonyPopBuffer_GetFirst_End_Process;
	{ //注意，这个大括号不是if 语句的大括号，是限定变量作用域
		char* pFirstTokenBegin = //寻找第一个Token 起始点
				TONY_POP_BUFFER_FIRST_TOKEN_BEGIN(m_pBuffer);
		STonyPopBufferTokenHead* pFirstTokenHead = //获得Token 头指针
				(STonyPopBufferTokenHead*) pFirstTokenBegin;
		char* pFirstTokenData = //获得Token 数据指针
				TONY_POP_BUFFER_TOKEN_DATA_BEGIN(pFirstTokenBegin);
		memcpy(szBuffer, pFirstTokenData, //拷贝数据到缓冲区
				pFirstTokenHead->m_nDataLength);
		nRet = pFirstTokenHead->m_nDataLength; //返回值设定
	}
	CTonyPopBuffer_GetFirst_End_Process: return nRet;
} //以Buffer 类方式获得第一个Token 数据，本函数破坏Buffer 原有内容
//由于Buffer 类本身是动态内存管理，因此，不存在缓冲区问题
int CTonyPopBuffer::GetFirst(CTonyBuffer* pBuffer) {
//防御性设计，
	if (!ICanWork())
		return 0;
	if (!pBuffer->SetSize(GetFirstTokenLength()))
		return 0;
	if (!pBuffer->m_nDataLength)
		return 0;
//调用上一函数，实现真实的GetFirst 功能。
	return GetFirst(pBuffer->m_pData, pBuffer->m_nDataLength);
}
//删除第一个元素，如果队列为空，删除会失败，返回false
bool CTonyPopBuffer::DeleteFirst(void) {
	bool bRet = false; //准备返回值
//防御性设计
	if (!ICanWork())
		goto CTonyPopBuffer_DeleteFirst_End_Porcess;
//如果队列为空，则返回
	if (!GetTokenCount())
		goto CTonyPopBuffer_DeleteFirst_End_Porcess;
	{ //同上，这不是if 的大括号，是限定变量作用域的大括号
		char* pFirstTokenBegin = //求出第一个Token 起始点
				TONY_POP_BUFFER_FIRST_TOKEN_BEGIN(m_pBuffer);
		STonyPopBufferTokenHead* pFirstTokenHead = //第一个Token 头指针
				(STonyPopBufferTokenHead*) pFirstTokenBegin;
		int nFirstTokenLength = //第一个Token 总长度
				TONY_POP_BUFFER_TOKEN_LENGTH(pFirstTokenHead->m_nDataLength);
		char* pSecondTokenBegin = //求出第二个Token 起始点
				pFirstTokenBegin + nFirstTokenLength;
		int nCopyBytesCount = //求出需要Move 的字节数
				m_pHead->m_nAllBytesCount - //队列总的Byte 数减去
						STonyPopBufferHeadSize - //队列头长度，再减去
						nFirstTokenLength; //第一个Token 的长度
		memcpy(pFirstTokenBegin, //Move 动作
				pSecondTokenBegin, //利用第二个Token 后面的数据
				nCopyBytesCount); //把第一个Token 从物理上覆盖掉
		m_pHead->m_nAllBytesCount -= //修订AllBytes，
				nFirstTokenLength; //减去第一个Token 长度
		m_pHead->m_nTokenCount--; //Token 总数-1
		bRet = true; //删除成功
	}
	CTonyPopBuffer_DeleteFirst_End_Porcess: return bRet;
}
//普通缓冲区方式，弹出第一个元素数据
int CTonyPopBuffer::GetAndDeleteFirst(char* szBuffer, int nBufferSize) {
	if (!ICanWork())
		return 0; //防御性设计
//请注意，这里不校验获得是否会成功，依赖上层程序保证缓冲区够大。
	int nRet = GetFirst(szBuffer, nBufferSize); //先获得第一个元素数据
	DeleteFirst(); //再删除第一个元素
	return nRet;
}
//Buffer 类方式，弹出第一个元素数据
int CTonyPopBuffer::GetAndDeleteFirst(CTonyBuffer* pBuffer) {
	if (!ICanWork())
		return 0; //防御性设计
	int nRet = GetFirst(pBuffer); //获得第一个元素数据
	DeleteFirst(); //删除第一个元素
	return nRet;
}
//枚举遍历所有数据，提交回调函数处理，并且删除数据，返回经过处理的Token 个数
//为了避免过多的动态内存分配细节，或者缓冲区的组织，本函数没有使用GetFirst
int CTonyPopBuffer::MoveAllData(_TONY_ENUM_DATA_CALLBACK pCallBack, //回调函数指针
		PVOID pCallParam) //代传的void*参数指针
		{
	int nMovedTokenCount = 0; //统计变量返回值
	bool bCallbackRet = true; //这是记录回调函数返回值的变量
	if (!pCallBack) //防御性设计，如果回调函数为空，不做事
		goto CTonyPopBuffer_MoveAllData_End_Process;
	if (!ICanWork()) //如果条件不满足，不做事
		goto CTonyPopBuffer_MoveAllData_End_Process;
	while (m_pHead->m_nTokenCount) //以TokenCount 为while 循环的条件
	{
		char* pFirstTokenBegin = //求得第一个Token 开始点
				TONY_POP_BUFFER_FIRST_TOKEN_BEGIN(m_pBuffer);
		STonyPopBufferTokenHead* pFirstTokenHead = //求得第一个Token 的头指针
				(STonyPopBufferTokenHead*) pFirstTokenBegin;
		char* pFirstTokenData = //求得第一个Token 的数据指针
				TONY_POP_BUFFER_TOKEN_DATA_BEGIN(pFirstTokenBegin);
//请注意此处回调，将第一个Token 数据传给回调函数，且获得回调函数返回值
		bCallbackRet = pCallBack(pFirstTokenData,
				pFirstTokenHead->m_nDataLength, pCallParam);
		DeleteFirst(); //删除第一个Token
		nMovedTokenCount++; //返回值+1
		if (!bCallbackRet)
			break; //如果回调返回假，中止循环
	}
	CTonyPopBuffer_MoveAllData_End_Process: return nMovedTokenCount; //返回处理的Token 个数
}

/////////////////////////////////////////////////////////////////////////
//******************************mem Queue类***************************///
/////////////////////////////////////////////////////////////////////////
//构造函数，初始化所有变量
CTonyXiaoMemoryQueue::CTonyXiaoMemoryQueue(CTonyLowDebug* pDebug,
		CTonyMemoryPoolWithLock* pMemPool, char* szAppName, int nMaxToken) {
	m_nMaxToken = nMaxToken; //保留最大Token 上线
	m_pLast = null; //注意，此处加速因子m_pLast 被设置为null
	m_pDebug = pDebug; //保留debug 对象指针
	m_pMemPool = pMemPool; //保留内存池指针
	SafeStrcpy(m_szAppName, szAppName, //保留AppName
			TONY_APPLICATION_NAME_SIZE);
	m_pHead = null; //注意，m_pHead 被设置为null，表示无Token
	m_nTokenCount = 0; //Token 计数器被设置为0
} //析构函数，清除所有的Token，释放申请的内存
CTonyXiaoMemoryQueue::~CTonyXiaoMemoryQueue() {
	if (m_pHead) {
		DeleteATokenAndHisNext (m_pHead); //此处递归释放
		m_pHead = null;
	}
}
bool CTonyXiaoMemoryQueue::ICanWork(void) {
	if (!m_pDebug)
		return false; //检查debug 对象指针
	if (!m_pMemPool)
		return false; //检查内存池指针
	return true;
}
void CTonyXiaoMemoryQueue::CleanAll(void) {
	if (!ICanWork())
		return; //防御性设计
	while (DeleteFirst()) {
	} //循环删除第一个对象，直到队列为空
//注意，本类的DeleteFirst 返回删除结果
}
int CTonyXiaoMemoryQueue::GetFirstLength(void) {
	int nRet = 0;
	if (m_pHead) //如果ICanWork 为否，m_pHead 必然为null
	{
		nRet = m_pHead->m_nDataLength; //取出第一个Token 的长度
	}
	return nRet;
}
//私有函数，打印某一个指定的Token，并且递归其后所有的Token
void CTonyXiaoMemoryQueue::PrintAToken(STonyXiaoQueueTokenHead* pToken) {
	if (!pToken)
		return; //防御型设计，递归结束标志
	TONY_XIAO_PRINTF(
			"Queue Token: pToken:%p, \
Buffer=%p, Length=%d, m_pNext=%p\n",
			pToken, //Token 指针
			pToken->m_pBuffer, //数据缓冲区指针
			pToken->m_nDataLength, //数据长度
			pToken->m_pNext); //下一Token 指针
	if (pToken->m_pBuffer) //以二进制方式格式化输出业务数据
		dbg_bin(pToken->m_pBuffer, pToken->m_nDataLength);
	if (pToken->m_pNext) //递归
		PrintAToken(pToken->m_pNext);
} //打印所有的Token 内容，公有函数入口
void CTonyXiaoMemoryQueue::PrintInside(void) {
	if (!ICanWork())
		return; //防御性设计
//输出队列关键信息
	TONY_XIAO_PRINTF("Queue: Token Count=%d, Head=0x%p, Last=0x%p\n",
			m_nTokenCount, //Token 总数
			m_pHead, //队列头Token 指针
			m_pLast); //队列尾Token 指针
	if (m_pHead) //从队列头开始递归
		PrintAToken (m_pHead);
}
//如果删除成功，返回真，否则返回假
bool CTonyXiaoMemoryQueue::DeleteATokenAndHisNext(
		STonyXiaoQueueTokenHead* pToken) //纯C 调用，需给定Token 指针
		{
	bool bRet = false; //准备返回值
	if (!ICanWork()) //防御性设计
		goto CTonyXiaoMemoryQueue_DeleteATokenAndHisNext_End_Process;
	if (pToken->m_pNext) //注意，如果有m_pNext，递归删除
	{
		DeleteATokenAndHisNext(pToken->m_pNext);
		pToken->m_pNext = null;
	}
	if (pToken->m_pBuffer) //开始删除本对象
	{
		m_pMemPool->Free(pToken->m_pBuffer); //向内存池释放数据缓冲区
		pToken->m_nDataLength = 0; //所有变量赋初值
		pToken->m_pBuffer = null;
	}
	m_pMemPool->Free((void*) pToken); //向内存池释放Token 头结构体缓冲区
	m_nTokenCount--; //Token 总计数器-1
	bRet = true;
	CTonyXiaoMemoryQueue_DeleteATokenAndHisNext_End_Process: m_pLast = null; //删除动作后，由于链表尾部发生变化
//加速因子失效，因此需要清空
	return bRet;
}
//创建一个Token 头内存区域，并初始化，返回Token 指针
STonyXiaoQueueTokenHead* CTonyXiaoMemoryQueue::GetAToken(void) {
	STonyXiaoQueueTokenHead* pToken = null; //准备返回值
	char* pTokenBuffer = null; //准备临时指针
	char szNameBuffer[256]; //准备说明文字缓冲区
	if (!ICanWork()) //防御性设计
		goto CTonyXiaoMemoryQueue_GetAToken_End_Process;
//格式化说明文字，注意其中用到了AppName
	SafePrintf(szNameBuffer, 256, "%s::Token_Head", m_szAppName);
//开始申请Token 头内存块
	pTokenBuffer = (char*) m_pMemPool->Malloc( //请注意对说明文字的使用
			STonyXiaoQueueTokenHeadSize, szNameBuffer);
	if (!pTokenBuffer) { //申请失败，返回null
		TONY_DEBUG("%s::GetAToken(): ma lloc new token fail!\n", m_szAppName);
		goto CTonyXiaoMemoryQueue_GetAToken_End_Process;
	} //强制指针类型转换，将申请到的二进制缓冲区指针转化成Token 头结构体指针
	pToken = (STonyXiaoQueueTokenHead*) pTokenBuffer;
	pToken->m_nDataLength = 0; //初始化过程，赋初值
	pToken->m_pNext = null;
	pToken->m_pBuffer = null;
	m_nTokenCount++; //Token 总数+1
	CTonyXiaoMemoryQueue_GetAToken_End_Process: return pToken;
}
//将数据保存到一个Token，如果该Token 已经保存数据，
//则按照链表顺序向下寻找，直到找到一个空的Token，把数据放置其中。
//如果成功，返回数据长度，如果失败，返回0
int CTonyXiaoMemoryQueue::AddLast2ThisToken(STonyXiaoQueueTokenHead* pToken, //需要处理的Token 头指针
		char* szData, //需要存储数据的指针
		int nDataLength) //需要存储的数据的长度
		{
	int nRet = 0; //准备返回值
	char szNameBuffer[256]; //准备说明文字缓冲区
	if (!ICanWork()) //防御性设计
		goto CTonyXiaoMemoryQueue_AddLast2ThisToken_End_Process;
	if (!pToken->m_pBuffer) {
//如果本Token 未包含有效数据，则保存到自己。
		SafePrintf(szNameBuffer, 256, //格式化内存块说明文字
				"%s::pToken->m_pBuffer", m_szAppName);
		pToken->m_pBuffer = (char*) m_pMemPool->Malloc( //向内存池申请内存块
				nDataLength, szNameBuffer);
		if (!pToken->m_pBuffer) { //申请失败，报警
			TONY_DEBUG(
					"%s::AddLast2ThisToken(): \
ma lloc pToken->m_pBuffer fail!\n",
					m_szAppName);
			goto CTonyXiaoMemoryQueue_AddLast2ThisToken_End_Process;
		}
		memcpy(pToken->m_pBuffer, szData, nDataLength); //拷贝业务数据到内存块
		pToken->m_nDataLength = nDataLength; //填充Token 头中的管理信息
		nRet = nDataLength; //给返回值赋值
		m_pLast = pToken; //保存加速因子(m_pLast 维护)
		goto CTonyXiaoMemoryQueue_AddLast2ThisToken_End_Process;
	} else {
//保存在下家
		if (!pToken->m_pNext) {
//如果指向下家的链指针为空，则利用GetAToken 创建一个头挂接
			pToken->m_pNext = GetAToken();
			if (!pToken->m_pNext) { //创建失败报警
				TONY_DEBUG(
						"%s::AddLast2ThisToken(): ma lloc \
pToken->m_pNext fail!\n",
						m_szAppName);
				goto CTonyXiaoMemoryQueue_AddLast2ThisToken_End_Process;
			}
		}
		if (pToken->m_pNext) //递归调用
			nRet = AddLast2ThisToken(pToken->m_pNext, szData, nDataLength);
	}
	CTonyXiaoMemoryQueue_AddLast2ThisToken_End_Process: return nRet;
}
//返回添加的数据长度，失败返回0
int CTonyXiaoMemoryQueue::AddLast(char* szData, //添加的数据指针
		int nDataLength, //数据长度
		int nLimit) //最大单元限制，-1 表示不限制
		{
	int nRet = 0; //准备返回值，初值为0
	if (!ICanWork()) //防御性设计，条件不符合，直接返回0
		goto CTonyXiaoMemoryQueue_AddLast_End_Process;
	if (0 >= nLimit) //应用限制值
			{ //这是无限制
		if (m_nMaxToken <= m_nTokenCount) //无限制时，以m_nMaxToken 作为边界限制
			goto CTonyXiaoMemoryQueue_AddLast_End_Process;
	} else {
//这是有限制
		if (nLimit <= m_nTokenCount) //如果有nLimit，则使用这个参数限制
			goto CTonyXiaoMemoryQueue_AddLast_End_Process;
	}
	if (!m_pHead) {
		m_pHead = GetAToken(); //这是链头第一次初始化
		if (!m_pHead) {
			TONY_DEBUG("%s::AddLast(): ma lloc m_pHead fail!\n", m_szAppName);
			goto CTonyXiaoMemoryQueue_AddLast_End_Process;
		}
	}
	if (m_pLast) //加速因子开始起作用，如果有值，直接跳入
	{
		nRet = AddLast2ThisToken(m_pLast, szData, nDataLength);
	} else if (m_pHead) //如果加速因子无值，按传统模式，遍历插入
	{
		nRet = AddLast2ThisToken(m_pHead, szData, nDataLength);
	}
	CTonyXiaoMemoryQueue_AddLast_End_Process: return nRet;
}
//针对普通缓冲区进行提取，应用层保证缓冲区够大
//返回提取数据的真实长度（不一定是上层缓冲区的大小）
int CTonyXiaoMemoryQueue::GetFirst(char* szBuffer, int nBufferSize) {
	int nRet = 0; //准备返回值变量
//此处开始防御性设计
	if (!ICanWork())
		goto CTonyXiaoMemoryQueue_GetFirst_End_Process;
	if (!m_pHead) //检查链表是否有数据
		goto CTonyXiaoMemoryQueue_GetFirst_End_Process;
	if (!m_pHead->m_pBuffer) //检查第一个Token 是否有数据
	{
		TONY_DEBUG("%s::GetFirst(): m_pHead->m_pBuffer=null\n", m_szAppName);
		goto CTonyXiaoMemoryQueue_GetFirst_End_Process;
	}
	if (m_pHead->m_nDataLength > nBufferSize) //检查给定的缓冲区是否足够
			{
		TONY_DEBUG("%s::GetFirst(): m_pHead->m_nDataLength > nBufferSize\n",
				m_szAppName);
		goto CTonyXiaoMemoryQueue_GetFirst_End_Process;
	}
	memcpy(szBuffer, m_pHead->m_pBuffer, m_pHead->m_nDataLength); //拷贝数据
	nRet = m_pHead->m_nDataLength; //返回值赋值
	CTonyXiaoMemoryQueue_GetFirst_End_Process: return nRet;
} //这是针对Buffer 类对象的提取函数
int CTonyXiaoMemoryQueue::GetFirst(CTonyBuffer* pBuffer) {
	if (!ICanWork())
		return 0; //防御性设计
	if (!pBuffer)
		return 0;
//调用上一函数完成功能
	return pBuffer->BinCopyFrom(m_pHead->m_pBuffer, m_pHead->m_nDataLength);
}
//删除第一个Token，有可能失败，因为队列可能为空，失败返回假，成功返回真
bool CTonyXiaoMemoryQueue::DeleteFirst(void) {
	bool bRet = false; //准备返回值
	STonyXiaoQueueTokenHead* pSecond = null; //准备指向第二Token 的临时指针
	if (!ICanWork()) //防御性设计
		goto CTonyXiaoMemoryQueue_DeleteFirst_End_Process;
	if (!m_pHead)
		goto CTonyXiaoMemoryQueue_DeleteFirst_End_Process;
//注意，先提取First 中保留的Socond 指针，做中间保护
	pSecond = m_pHead->m_pNext;
//m_pHead 的m_pNext 赋为空值，这是割裂与队列其他元素的连接关系
	m_pHead->m_pNext = null;
//然后调用DeleteATokenAndHisNext 完成删除，由于上面的割裂，因此不会影响其他Token
	bRet = DeleteATokenAndHisNext(m_pHead);
	if (bRet) {
		m_pHead = pSecond; //重新给m_pHead 挂接第二Token
//完成对原有First 的挤出工作
//此时，原有Second 变为First
		if (!m_pHead) //这里有一个特例，
			m_pLast = null; //如果本次删除把该队列删空，
//需要清除加速因子，避免下次操作失败
	} else { //如果删除失败（这种情况一般不太可能，属于保护动作，不会真实执行）
//将队列恢复原状，起码不至于在本函数内部造成崩溃，或内存泄漏，但打印报警
		TONY_DEBUG("%s::DeleteFirst(): delete m_pHead fail!\n", m_szAppName);
		m_pHead->m_pNext = pSecond; //删除失败，还得恢复回去
	}
	CTonyXiaoMemoryQueue_DeleteFirst_End_Process: return bRet;
}
int CTonyXiaoMemoryQueue::GetAndDeleteFirst(char* szBuffer, int nBufferSize) {
	int nRet = GetFirst(szBuffer, nBufferSize);
	if (nRet)
		DeleteFirst();
	return nRet;
}
int CTonyXiaoMemoryQueue::GetAndDeleteFirst(CTonyBuffer* pBuffer) {
	int nRet = GetFirst(pBuffer);
	if (nRet)
		DeleteFirst();
	return nRet;
}
//返回弹出的数据总长度，但请注意，不是PopBuffer 的AllBytes，仅仅是业务数据长度和
int CTonyXiaoMemoryQueue::PopFromFirst4TonyPopBuffer(CTonyPopBuffer* pPopBuffer) //需传入PopBuffer 指针
		{
	int nRet = 0; //准备返回值变量
	if (!ICanWork()) //防御性设计
		goto CTonyXiaoMemoryQueue_PopFromFirst4TonyPopBuffer_End_Process;
	if (!m_pHead)
		goto CTonyXiaoMemoryQueue_PopFromFirst4TonyPopBuffer_End_Process;
	if (!m_pHead->m_pBuffer) //这也是防御性设计，检索First 是否有数据
	{
		TONY_DEBUG(
				"%s::PopFromFirst4TonyPopBuffer(): \
m_pHead->m_pBuffer=null\n",
				m_szAppName);
		goto CTonyXiaoMemoryQueue_PopFromFirst4TonyPopBuffer_End_Process;
	}
	if (!m_pHead->m_nDataLength) //防御性设计，检索First 数据尺寸是否合法
	{
		TONY_DEBUG(
				"%s::PopFromFirst4TonyPopBuffer(): \
m_pHead->m_nDataLength=0\n",
				m_szAppName);
		goto CTonyXiaoMemoryQueue_PopFromFirst4TonyPopBuffer_End_Process;
	}
	if (!pPopBuffer) //检查给定的PopBuffer 是否合法
	{
		TONY_DEBUG("%s::PopFromFirst4TonyPopBuffer(): pPopBuffer=null\n",
				m_szAppName);
		goto CTonyXiaoMemoryQueue_PopFromFirst4TonyPopBuffer_End_Process;
	} //没有使用GetAndDeleteFirst 之类的函数，而是直接访问结构体内部变量
//就是预防如果PopBuffer 满了，弹出来的数据不好处理，这样也减少无谓的动态内存分配
	nRet = pPopBuffer->AddLast(m_pHead->m_pBuffer, m_pHead->m_nDataLength);
	if (m_pHead->m_nDataLength != nRet) {
//这是buffer 装满了
		goto CTonyXiaoMemoryQueue_PopFromFirst4TonyPopBuffer_End_Process;
	}
	if (!DeleteFirst()) //删除First
	{
		TONY_DEBUG("%s::PopFromFirst4TonyPopBuffer(): DeleteFirst fail\n",
				m_szAppName);
		goto CTonyXiaoMemoryQueue_PopFromFirst4TonyPopBuffer_End_Process;
	}
	if (m_pHead) //注意此处的递归逻辑，继续向下弹
		nRet += PopFromFirst4TonyPopBuffer(pPopBuffer);
	CTonyXiaoMemoryQueue_PopFromFirst4TonyPopBuffer_End_Process: return nRet;
}
//向普通缓冲区，以PopBuffer 方式打包信令
int CTonyXiaoMemoryQueue::PopFromFirst(char* szBuffer, int nBufferSize) {
	int nCopyBytes = 0; //准备返回值
//准备一个PopBuffer 对象，注意，粘合作用。
	CTonyPopBuffer PopBuffer(szBuffer, nBufferSize);
	if (!ICanWork()) //防御性设计
		goto CTonyXiaoMemoryQueue_PopFromFirst_End_Process;
	if (m_pHead) { //调用上一函数实现打包
		nCopyBytes = PopFromFirst4TonyPopBuffer(&PopBuffer);
		if (nCopyBytes) //如果弹出了数据（数据长度!=0）
//取PopBuffer 的AllBytes 作为返回值
			nCopyBytes = PopBuffer.GetAllBytes(); //这就是实际需要发送的字节数
	}
	CTonyXiaoMemoryQueue_PopFromFirst_End_Process: return nCopyBytes;
}
//将受到的一段PopBuffer 格式化的信令编组，推入MemQueue 的队列末尾
//返回推入的数据字节常数，失败返回0
int CTonyXiaoMemoryQueue::Push2Last(char* szData, int nDataLength) {
	int nRet = 0;
	CTonyPopBuffer PopBuffer(szData, nDataLength, false); //此处粘合
	if (!ICanWork()) //防御性设计
		goto CTonyXiaoMemoryQueue_Push2Last_End_Process;
//开始调用PopBuffer 的功能，实现遍历循环
//请注意回调函数和this 指针参数
	PopBuffer.MoveAllData(PushDataCallback, this);
	CTonyXiaoMemoryQueue_Push2Last_End_Process: return nRet;
}
//回调函数，返回bool 值，如果返回false，遍历循环将退出，信令可能会丢失。
bool CTonyXiaoMemoryQueue::PushDataCallback(char* szData, //拆解出来的信令地址
		int nDataLength, //信令数据长度
		void* pCallParam) //代传的参数指针，就是前面的this
		{
//笔者的习惯，一般类内回调函数，均以this 指针作为透传参数
//在回调函数内部，一般称之为pThis，以区别于普通成员函数的this
//回调函数一进入，第一件事就是强制指针类型转换，创建pThis，方便调用。
	CTonyXiaoMemoryQueue* pThis = (CTonyXiaoMemoryQueue*) pCallParam;
//此处为调用实例，pThis 指针就是this，因此，调用的是本对象的AddLast
	int nRet = pThis->AddLast(szData, nDataLength);
//成功，返回thre，遍历继续，直到数据遍历结束
	if (nDataLength == nRet)
		return true;
	else { //失败，可能是队列满了，返回false，终止遍历，
		pThis->TONY_DEBUG("%s::PushDataCallback(): I am full!\n",
				pThis->m_szAppName);
		return false;
	}
}
//将指定Token 的数据写入文件
void CTonyXiaoMemoryQueue::WriteAToken2File(STonyXiaoQueueTokenHead* pToken, //指定的Token 指针
		FILE* fp) //指定的文件指针
		{
	if (!ICanWork())
		return; //防御性设计
	if (!fp)
		return;
	if (!pToken)
		return;
	if (!pToken->m_pBuffer)
		return;
	if (!pToken->m_nDataLength)
		return;
//写入磁盘，由于磁盘写入，通常发生在服务器即将退出期间，这已经是数据保护最大的努力
//此时磁盘写入再出现bug，没有任何办法再保护数据，只能丢失信令
//因此，此处不再做校验，如果磁盘写入失败，即丢失信令
//先写入每个Token 的数据长度
	fwrite((void*) &(pToken->m_nDataLength), sizeof(int), 1, fp);
//再写入每个Token 的实际数据
	fwrite((void*) pToken->m_pBuffer, sizeof(char), pToken->m_nDataLength, fp);
	if (pToken->m_pNext) //递归到下一Token
		WriteAToken2File(pToken->m_pNext, fp);
} //这是公有方法入口函数
void CTonyXiaoMemoryQueue::Write2File(char* szFileName) //需给出文件名
		{
	FILE* fp = null;
	if (!ICanWork())
		return; //防御性设计
	if (!m_pHead)
		return;
	if (!GetTokenCount())
		return; //如果队列为空，直接返回
	fp = fopen(szFileName, "wb"); //打开文件
	if (fp) {
//首先，将队列控制信息写入磁盘，以便读回时直接使用
		fwrite((void*) &m_nTokenCount, sizeof(int), 1, fp);
//开始调用上述递归函数，开始逐Token 写入
		WriteAToken2File(m_pHead, fp);
		fclose(fp); //关闭文件
	}
}
//从一个磁盘Dump 文件中读入数据，返回读入的Token 总数
int CTonyXiaoMemoryQueue::ReadFromFile(char* szFileName) {
	FILE* fp = null;
	int n = 0;
	int i = 0;
	int nReadTokenCount = 0;
	int nDataLength = 0;
	char* pTempBuffer = null;
	char szNameBuffer[256];
	if (!ICanWork()) //防御性设计
		goto CTonyXiaoMemoryQueue_ReadFromFile_End_Process;
	SafePrintf(szNameBuffer, 256, //构建内存申请说明文字
			"%s::ReadFromFile::pTempBuffer", m_szAppName);
//申请临时缓冲区，由于临时缓冲区需要在循环中多次ReMalloc，因此，开始只申请1Byte 即可
	pTempBuffer = (char*) m_pMemPool->Malloc(1, szNameBuffer);
	fp = fopen(szFileName, "rb"); //打开文件
	if (!fp) //失败则跳转返回
		goto CTonyXiaoMemoryQueue_ReadFromFile_End_Process;
//读入队列控制头，即TokenCount 信息
	n = fread((void*) &nReadTokenCount, sizeof(int), 1, fp);
	if (1 > n) {
		TONY_DEBUG("%s::ReadFromFile(): read token count fail!\n", m_szAppName);
		goto CTonyXiaoMemoryQueue_ReadFromFile_End_Process;
	}
	for (i = 0; i < nReadTokenCount; i++) //开始逐一读出各个Token
			{
//首先读出当前Token 的数据长度
		n = fread((void*) &(nDataLength), sizeof(int), 1, fp);
		if (1 > n) {
			TONY_DEBUG("%s::ReadFromFile(): %d / %d, read data length fail!\n",
					m_szAppName, i, nReadTokenCount);
			goto CTonyXiaoMemoryQueue_ReadFromFile_End_Process;
		}
		if (0 > nDataLength) {
			TONY_DEBUG("%s::ReadFromFile(): %d / %d, nDataLength=%d < 0!\n",
					m_szAppName, i, nReadTokenCount, nDataLength);
			goto CTonyXiaoMemoryQueue_ReadFromFile_End_Process;
		} //调用ReMalloc，根据读入的nDataLength 重新分配缓冲区，以便读入数据
		pTempBuffer = (char*) m_pMemPool->ReMalloc(pTempBuffer, //原有的临时缓冲区地址
				nDataLength, //新的数据长度
				false); //由于这个缓冲区马上被覆盖
//因此无需拷贝旧数据
		if (!pTempBuffer) {
			TONY_DEBUG("%s::ReadFromFile(): rema lloc pTempBuffer fail!\n",
					m_szAppName);
			goto CTonyXiaoMemoryQueue_ReadFromFile_End_Process;
		} //准备缓冲区成功，开始读入该Token 数据
		n = fread((void*) pTempBuffer, sizeof(char), nDataLength, fp);
		if (nDataLength > n) {
			TONY_DEBUG("%s::ReadFromFile(): read data fail!\n", m_szAppName);
			goto CTonyXiaoMemoryQueue_ReadFromFile_End_Process;
		} //读入成功，AddLast 添加到最后
		if (!AddLast(pTempBuffer, nDataLength))
			break;
	}
	CTonyXiaoMemoryQueue_ReadFromFile_End_Process: //出错跳转标签
	if (pTempBuffer) //清除临时缓冲区
	{
		m_pMemPool->Free(pTempBuffer);
		pTempBuffer = null;
	}
	if (fp) //关闭文件
	{
		fclose(fp);
		fp = null;
	}
	return nReadTokenCount; //返回读入的Token 总数
}
/////////////////////////////////////////////////////////////////////////
//*********************mem Queue线程安全封装类***************************///
/////////////////////////////////////////////////////////////////////////
//构造函数，请注意，参数和MemQueue 完全一样
CTonyMemoryQueueWithLock::CTonyMemoryQueueWithLock(CTonyLowDebug* pDebug,
		CTonyMemoryPoolWithLock* pMemPool, char* szAppName, int nMaxToken) {
//保存AppName
	SafeStrcpy(m_szAppName, szAppName, TONY_APPLICATION_NAME_SIZE);
	m_pMemPool = pMemPool; //保存内存池指针，析构函数要用
	m_pQueue = new CTonyXiaoMemoryQueue(pDebug, //实例化封装对象
			pMemPool, m_szAppName, nMaxToken);
	if (m_pQueue) {
		char szNameBuffer[256];
//如果实例化成功，则试图在内存池注册它，以实现指针管理
//注册前，先利用AppName 构造说明文字
		SafePrintf(szNameBuffer, 256, "%s::m_pQueue", m_szAppName);
		m_pMemPool->Register(m_pQueue, szNameBuffer);
	}
} //析构函数，摧毁封装对象爱那个
CTonyMemoryQueueWithLock::~CTonyMemoryQueueWithLock() {
	m_Lock.EnableWrite(); //此处是带写函数，因此使用写锁
	{
		if (m_pQueue) {
			m_pMemPool->UnRegister(m_pQueue); //这部反注册很重要，否则内存池报警
			delete m_pQueue; //摧毁对象
			m_pQueue = null; //习惯，变量摧毁后立即赋初值
		}
	}
	m_Lock.DisableWrite();
}
bool CTonyMemoryQueueWithLock::ICanWork(void) {
	if (!m_pMemPool)
		return false;
	if (!m_pQueue)
		return false;
	bool bRet = false;
	m_Lock.AddRead();
	{
		bRet = m_pQueue->ICanWork();
	}
	m_Lock.DecRead();
	return bRet;
}
int CTonyMemoryQueueWithLock::GetFirst(CTonyBuffer* pBuffer) {
	int nRet = 0;
	m_Lock.AddRead();
	{
		nRet = m_pQueue->GetFirst(pBuffer);
	}
	m_Lock.DecRead();
	return nRet;
}
int CTonyMemoryQueueWithLock::GetFirst(char* szBuffer, int nBufferSize) {
	int nRet = 0;
	m_Lock.AddRead();
	{
		nRet = m_pQueue->GetFirst(szBuffer, nBufferSize);
	}
	m_Lock.DecRead();
	return nRet;
}
int CTonyMemoryQueueWithLock::GetFirstLength(void) {
	int nRet = 0;
	m_Lock.AddRead();
	{
		nRet = m_pQueue->GetFirstLength();
	}
	m_Lock.DecRead();
	return nRet;
}
int CTonyMemoryQueueWithLock::GetTokenCount(void) {
	int nRet = 0;
	m_Lock.AddRead();
	{
		nRet = m_pQueue->GetTokenCount();
	}
	m_Lock.DecRead();
	return nRet;
}
void CTonyMemoryQueueWithLock::Write2File(char* szFileName) {
	m_Lock.AddRead();
	{
		m_pQueue->Write2File(szFileName);
	}
	m_Lock.DecRead();
}
void CTonyMemoryQueueWithLock::PrintInside(void) {
	m_Lock.AddRead();
	{
		m_pQueue->PrintInside();
	}
	m_Lock.DecRead();
}
int CTonyMemoryQueueWithLock::AddLast(char* szData, int nDataLength) {
	int nRet = 0;
	m_Lock.EnableWrite();
	{
		nRet = m_pQueue->AddLast(szData, nDataLength);
	}
	m_Lock.DisableWrite();
	return nRet;
}
bool CTonyMemoryQueueWithLock::DeleteFirst(void) {
	bool bRet = 0;
	m_Lock.EnableWrite();
	{
		bRet = m_pQueue->DeleteFirst();
	}
	m_Lock.DisableWrite();
	return bRet;
}
int CTonyMemoryQueueWithLock::GetAndDeleteFirst(char* szBuffer,
		int nBufferSize) {
	int nRet = 0;
	m_Lock.EnableWrite();
	{
		nRet = m_pQueue->GetAndDeleteFirst(szBuffer, nBufferSize);
	}
	m_Lock.DisableWrite();
	return nRet;
}
int CTonyMemoryQueueWithLock::PopFromFirst(char* szBuffer, int nBufferSize) {
	int nRet = 0;
	m_Lock.EnableWrite();
	{
		nRet = m_pQueue->PopFromFirst(szBuffer, nBufferSize);
	}
	m_Lock.DisableWrite();
	return nRet;
}
int CTonyMemoryQueueWithLock::Push2Last(char* szData, int nDataLength) {
	int nRet = 0;
	m_Lock.EnableWrite();
	{
		nRet = m_pQueue->Push2Last(szData, nDataLength);
	}
	m_Lock.DisableWrite();
	return nRet;
}
void CTonyMemoryQueueWithLock::CleanAll(void) {
	m_Lock.EnableWrite();
	{
		m_pQueue->CleanAll();
	}
	m_Lock.DisableWrite();
}
int CTonyMemoryQueueWithLock::ReadFromFile(char* szFileName) {
	int nRet = 0;
	m_Lock.EnableWrite();
	{
		nRet = m_pQueue->ReadFromFile(szFileName);
	}
	m_Lock.DisableWrite();
	return nRet;
}
