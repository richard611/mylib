#include "std.h"
#include "Socket.h"
#include "Debug.h"
#include "MutexLock.h"
#include "MrswLock.h"
#include "TonyLowDebug.h"
#include "MemoryManager.h"

/////////////////////////////////////////////////////////////////////
//************************内存栈基本单元*****************************///
/////////////////////////////////////////////////////////////////////
CTonyMemoryStackToken::CTonyMemoryStackToken(int nBlockSize,
		CTonyLowDebug* pDebug) {
	m_pDebug = pDebug; //保存Debug 对象指针
	m_ulBlockSize = (ULONG) nBlockSize; //保存本对象管理的内存块尺寸
	m_nBlockOutSide.Set(0); //统计变量初始化
	m_nBlockInSide.Set(0);
	m_Lock.EnableWrite(); //注意单写多读锁的使用，这是写锁
	{
		m_pFirstSon = null; //指针变量初始化
		m_pNext = null; //注意，这是在锁保护下进行的
	}
	m_Lock.DisableWrite();
}
CTonyMemoryStackToken::~CTonyMemoryStackToken() {
	if (m_nBlockOutSide.Get()) {
//注意此处，当检测到还有内存块在外被应用程序使用，
//而本对象又需要析构，即内存池停止服务时，报警提醒程序员。
		TONY_DEBUG("Tony Memory Stack: lost %d * %d\n", m_ulBlockSize,
				m_nBlockOutSide.Get());
	}
	m_Lock.EnableWrite();
	{
//此处，在锁保护下，摧毁所有的右枝和左枝，这是递归运算
		if (m_pFirstSon)
			DestroySon (m_pFirstSon);
		m_pFirstSon = null;
		if (m_pNext)
			delete m_pNext;
		m_pNext = null;
	}
	m_Lock.DisableWrite();
}

//Malloc 逻辑
void* CTonyMemoryStackToken::Malloc(int nSize, CMRSWint& nAllBlockCount,
		CMRSWint& nMemoryUse) {
	void* pRet = null; //准备返回的指针
	STonyMemoryBlockHead* pNew = null; //中间变量
	//请注意这个宏的使用
	//根据一个应用程序数据块的长度，计算一个内存块的真实大小，即n+8
	//#define TONY_MEM_BLOCK_SIZE(nDataLength) \
	// (nDataLength+STonyMemoryBlockHeadSize)
	//这是申请大小+8，再和本对象管理大小比较，以此修订管理数据带来的偏差
	//这里还有一个技巧，m_ulBlockSize 没有使用锁保护，这是因为m_ulBlockSize
	//从构造函数填充后，所有的使用都是只读的，不再需要锁保护，以简化锁操作
	if ((TONY_MEM_BLOCK_SIZE(nSize)) < m_ulBlockSize) {
		//这表示本对象的大小可以满足申请需求，申请将从本节点右枝完成
		m_Lock.EnableWrite(); //加锁
		{
			//判断是否有空闲的内存块备用
			if (!m_pFirstSon) {
				//如果没有，需要向系统新申请一个内存块
				//注意这里的强制指针类型转换，申请的内存块，在本函数直接
				//转换成管理结构体指针，以便操作
				//注意申请的大小就是m_ulBlockSize，因此
				//本对象申请的所有内存块，都是一样大小的。
				pNew = (STonyMemoryBlockHead*) malloc(m_ulBlockSize);
				if (pNew) //安全检查
				{
					//统计函数+1，因为这个内存块马上会分配出去，
					//因此，修订m_nBlockOutSide+1，InSide 不加。
					m_nBlockOutSide.Add();
					//这是帮助上层统计总内存字节数，注意，这是传址调用，数据会上传
					nMemoryUse.Add(m_ulBlockSize);
					//初始化新申请的内存块，填充大小信息
					//注意，下面的Free 要用到这个信息查找对应的左枝节点。
					pNew->m_ulBlockSize = m_ulBlockSize;
					//由于链表指针只有在管理时使用，分配出去的，暂时清空
					pNew->m_pNext = null;
					//这里最关键，请注意，使用了宏计算，p1=p0+8，求得p1，
					//返回给应用程序使用
					pRet = TONY_MEM_BLOCK_DATA(pNew);
					//统计变量，内存块总数+1
					nAllBlockCount.Add();
				} //如果申请失败，则直接返回null 给应用程序
			} else {
				//这是有空闲内存块的情况
				//直接提取链表第一块，也就是栈上最新加入的内存块
				pNew = m_pFirstSon;
				//这是指针修订，注意，本类中m_pFirstSon 已经指向了原来第二块内存
				m_pFirstSon = pNew->m_pNext;
				//同上，分配出去的内存块，m_pNext 无用，清空
				pNew->m_pNext = null;
				//求出p1=p0+8，返回给应用程序使用
				pRet = TONY_MEM_BLOCK_DATA(pNew);
				//注意此处，这是把内部的内存块再分配重用
				//因此，OutSide+1，InSide-1
				m_nBlockOutSide.Add();
				m_nBlockInSide.Dec();
			}
		}
		m_Lock.DisableWrite(); //解		锁
	} else {
		m_Lock.EnableWrite(); //加写锁
		{
			//这是检测兄弟节点是否已经创建，如果没有，则创建之。
			//注意，此处对m_pNext 有写关系，因此，加写锁。
			if (!m_pNext) {
				m_pNext = new CTonyMemoryStackToken(m_ulBlockSize * 2,
						m_pDebug);
			}
		}
		m_Lock.DisableWrite(); //解写锁
		//此处锁读写模式转换非常重要，下面专门论述
		m_Lock.AddRead(); //加读锁
		{
			//将请求传递给兄弟节点的Malloc 函数处理
			if (m_pNext)
				pRet = m_pNext->Malloc(nSize, nAllBlockCount, nMemoryUse);
			//如果兄弟节点创建失败，表示系统内存分配也失败，返回空指针给应用程序
		}
		m_Lock.DecRead(); //解	读	锁
	}
	return pRet;
}
//Free 函数，允许代入CloseFlag 标志，注意，外部传入的指针为p1，需要逆求p0=p1-8
//在某种情况下，Free 可能会失败，因此，需要返回一个bool 值，表示释放结果。
bool CTonyMemoryStackToken::Free(void* pPoint, bool bCloseFlag) {
	bool bRet = false; //准备返回值变量
	//注意这里，这个pOld，已经是计算的p0=p1-8
	STonyMemoryBlockHead* pOld = TONY_MEM_BLOCK_HEAD(pPoint);
	//首先检测指定内存块大小是否由本对象管理。
	if (m_ulBlockSize == pOld->m_ulBlockSize) {
		//此处是判断内存块对象是否超限，如果超出限额，直接释放，不做重用处理。
		//因此，对于超限的内存块，其管理节点对象总是存在的，但对象的右枝为空。
		if (TONY_XIAO_MEMORY_STACK_MAX_SAVE_BLOCK_SIZE <= m_ulBlockSize) {
			free(pOld);
			m_nBlockOutSide.Dec(); //注意，这里修订OutSide 统计变量。
		} else if (bCloseFlag) { //当CloseFlag 为真，不再推入右枝，直接释放
			free(pOld);
			m_nBlockOutSide.Dec();
		} else { //当所有条件不满足，正常推入右枝栈，准备重用内存块
			m_Lock.EnableWrite(); //加写锁
			{ //请注意这里，典型的压栈操作，新加入内存块，放在第一个，
			  //原有的内存块，被链接到新内存块的下一个，先进先出
				pOld->m_pNext = m_pFirstSon;
				m_pFirstSon = pOld;
			}
			m_Lock.DisableWrite(); //解除写锁
			m_nBlockOutSide.Dec(); //修订统计变量
			m_nBlockInSide.Add();
		} //请注意返回值的位置，只要是本节点处理，都会返回true。
		bRet = true;
	} else { //这里有一个默认推定，请大家关注：
		//当一个程序，所有的动态内存申请都是由本内存池管理是，
		//内存块一定是被本内存池分配，因此，当内存块释放时，
		//其对应的左枝节点，即内存管理单元节点，一定存在
		//因此，Free 不再处理m_pNext 的创建问题，而是直接调用
		m_Lock.AddRead(); //加读锁，原因前面已经论述
		{ //此处递归操作
			if (m_pNext)
				bRet = m_pNext->Free(pPoint, bCloseFlag);
		}
		m_Lock.DecRead(); //解读锁
	}
	return bRet;
}

//摧毁所有的子节点
void CTonyMemoryStackToken::DestroySon(STonyMemoryBlockHead* pSon) { //本函数是典型的对象内私有函数，被其他线程安全的函数调用，因此，内部不加锁。
	STonyMemoryBlockHead* pObjNow = pSon; //中间变量
	STonyMemoryBlockHead* pOnjNext = null;
	while (1) //循环方式
	{
		if (!pObjNow)
			break; //循环跳出点
		pOnjNext = pObjNow->m_pNext;
		free(pObjNow);
		m_nBlockInSide.Dec();
		pObjNow = pOnjNext;
	}
}

//内存管理单元的信息输出函数
void CTonyMemoryStackToken::PrintStack(void) {
//这是一个典型的递归函数，一次打印左枝上所有的内存管理单元的内容
	if (m_nBlockInSide.Get() + m_nBlockOutSide.Get()) { //有值则打印，无值不输出
		TONY_XIAO_PRINTF(" [%ld] stack: all=%d, out=%d, in=%d\n", m_ulBlockSize, //这是提示内存块的大小
				m_nBlockInSide.Get() + m_nBlockOutSide.Get(), //这是所有内存块的总数
				m_nBlockOutSide.Get(), //分配出去的内存块
				m_nBlockInSide.Get()); //内部保存备用的内存块
	}
	m_Lock.AddRead(); //加读锁
	if (m_pNext) {
		m_pNext->PrintStack(); //注意，这里递归
	}
	m_Lock.DecRead(); //解读锁
}

///////////////////////////////////////////////////////////////////////////
//*****************************内存栈************************************///
///////////////////////////////////////////////////////////////////////////
//构造函数
CTonyMemoryStack::CTonyMemoryStack(CTonyLowDebug* pDebug) {
	m_CloseFlag.Set(false); //关闭标志清空
	m_pDebug = pDebug; //保存debug 对象指针
	m_pMaxPoint.Set(0); //统计变量赋初值
	m_nAllBlockCount.Set(0);
	m_nMemoryUse.Set(0);
	m_pHead = new CTonyMemoryStackToken( //以最小内存块尺寸16Bytes，
			TONY_XIAO_MEMORY_STACK_BLOCK_MIN, //构造左枝第一个节点
			m_pDebug);
//注意，在整个运行期间，左枝节点只创建，不摧毁，因此，不会因为
//动态对象而造成内存碎片
} //析构函数
CTonyMemoryStack::~CTonyMemoryStack() {
//退出时，汇报一下使用的最大指针，方便程序员观察
	TONY_DEBUG("Memory Stack: Max Point= 0x%p\n", m_pMaxPoint.Get());
//清除左枝，这是递归删除，由每个节点的析构完成
	if (m_pHead) {
		delete m_pHead;
		m_pHead = null;
	}
} //公有方法，设置Close 标志
void CTonyMemoryStack::SetCloseFlag(bool bFlag) {
	m_CloseFlag.Set(true);
}

void* CTonyMemoryStack::Malloc(int nSize) {
	void* pRet = null;
	if (0 >= nSize) //防御性设计，申请长度<=0 的内存空间无意义
			{
		TONY_DEBUG("CTonyMemoryStack::Malloc(): ERROR! nSize=%d\n", nSize);
		return pRet;
	}
	if (m_pHead) {
		//注意，此处进入递归分配，上一小节逻辑，递归到合适的管理单元分配
		pRet = m_pHead->Malloc(nSize, m_nAllBlockCount, m_nMemoryUse);
		//统计功能，统计最大的指针
		if (m_pMaxPoint.Get() < (int) pRet)
			m_pMaxPoint.Set((int) pRet);
	}
	return pRet;
}
bool CTonyMemoryStack::Free(void* pPoint) {
	bool bRet = false;
	if (m_pHead) //递归调用左枝节点上的Free，完成Free 功能
	{
		bRet = m_pHead->Free(pPoint, m_CloseFlag.Get());
	}
	return bRet;
}
//ReMalloc 功能，改变一个指针指向的内存区的尺寸。
//假定该指针是本内存池分配的，因此，根据给出的指针p1，可以逆推算出p0=p1-8，
//并由此获得原有尺寸的真实大小。
void* CTonyMemoryStack::ReMalloc(void* pPoint, //原有指针，p1
		int nNewSize, //新尺寸
		bool bCopyOldDataFlag) //拷贝旧数据标志
		{
	void* pRet = null;
	STonyMemoryBlockHead* pOldToken = null; //旧的p0
	int nOldLen = 0; //旧的长度
	if (0 >= nNewSize) //防御性设计，防止申请非法的长度（<=0）
			{ //请注意打印格式，以函数名开始，方便程序员观察Debug 信息时定位
		TONY_DEBUG("CTonyMemoryStack::ReMalloc(): ERROR! nNewSize=%d\n",
				nNewSize);
		goto CTonyMemoryStack_ReMalloc_Free_Old;
	} //请注意这里，调用宏计算p0=p1-8
	pOldToken = TONY_MEM_BLOCK_HEAD(pPoint);
	//求的原内存块的大小
	nOldLen = pOldToken->m_ulBlockSize;
	//比较新的长度和内存块总长度的大小，
	//特别注意，NewSize 被宏计算+8，求的是内存块大小，这是修订误差
	if (TONY_MEM_BLOCK_SIZE(nNewSize) <= (ULONG) nOldLen) {
		//我们知道，内存栈管理的内存块都是已经取模的内存块，
		//其长度通常都是16Bytes 的整倍数，并且比应用程序申请的内存要大
		//因此，很可能应用程序新申请的内存大小，并没有超过原内存块本身的大小
		//此时，就可以直接将原内存块返回继续使用，不会因此导致内存溢出等bug
		//比如，应用程序第一次申请一个260Bytes 的内存块，根据内存取模原则
		//内存池会分配一个512Bytes 的内存块给其使用，有效使用空间512-8=504Bytes
		//而当第二次，应用程序希望使用一个300Bytes 的内存区，还是小于504Bytes
		//此时，ReMalloc 会返回旧指针，让其继续使用，而不是重新申请，以提高效率。
		//另外，从这个逻辑我们也可以得知，如果调整的新尺寸本来就比原空间小
		//比如第二次调整的尺寸不是300Bytes，而是100Bytes，
		//也会因为这个原因，继续使用原内存，避免二次分配
		//这是一个典型的空间换时间优化，以一点内存的浪费，避免二次分配，提升效率
		//由于这是原内存返回，自然保留原有数据，本函数无需拷贝数据
		pRet = pPoint;
		//请注意这里，跳过了第一跳转点，直接跳到正常结束
		//即传入的指针不会被Free，而是二次使用。
		goto CTonyMemoryStack_ReMalloc_End_Process;
	} //当确定新的尺寸比较大，原有内存无法承受，则必须调用内存管理单元，重新申请一块
	pRet = m_pHead->Malloc(nNewSize, m_nAllBlockCount, m_nMemoryUse);
	//当新内存申请成功，根据拷贝表示拷贝数据
	if ((pRet) && (pPoint))
		if (bCopyOldDataFlag)
			memcpy(pRet, pPoint, nOldLen);
	CTonyMemoryStack_ReMalloc_Free_Old:
	//第一跳转点，出错时，或正确结束时，均Free 旧指针
	m_pHead->Free(pPoint, m_CloseFlag.Get());
	CTonyMemoryStack_ReMalloc_End_Process:
	//正常退出跳转点，返回新指针
	return pRet;
}

//打印关键变量的值
void CTonyMemoryStack::PrintInfo(void) {
	TONY_XIAO_PRINTF("block=%d, use=%d kbytes, biggest=%p\n",
			m_nAllBlockCount.Get(), //所有内存块的总数
			m_nMemoryUse.Get() / 1024, //总内存使用大小（KBytes）
			m_pMaxPoint.Get()); //最大指针
} //递归调用内存管理单元的对应方法，打印整个树的情况。
void CTonyMemoryStack::PrintStack(void) {
	if (m_pHead)
		m_pHead->PrintStack();
}

////////////////////////////////////////////////////////////////////////
//*****************************内存注册模块*****************************//
////////////////////////////////////////////////////////////////////////

//这里使用了一个宏TONY_CLEAN_CHAR_BUFFER，其定义如下：

//构造函数
CMemoryRegister::CMemoryRegister(CTonyLowDebug* pDebug) {
	m_pDebug = pDebug;
//初始化变量
	m_pMaxPoint = null;
	m_nUseMax = 0;
	m_nPointCount = 0;
//请注意这里，我们的结构体中，并没有单独设计标示使用与否的变量
//因此，一般是以保存的指针是否为null 来表示结构体是否被使用
//这就要求，初始化时，先将所有的内存块指针均置为null，方便后续函数判读
	int i = 0;
	for (i = 0; i < TONY_MEMORY_REGISTER_MAX; i++) {
		m_RegisterArray[i].m_pPoint = null;
		TONY_CLEAN_CHAR_BUFFER(m_RegisterArray[i].m_szInfo);
	}
}
//析构函数
CMemoryRegister::~CMemoryRegister() {
	int i = 0;
	m_Lock.Lock(); //加锁
	{
//打印统计的最大指针，提醒程序员
		TONY_DEBUG("CMemoryRegister: Max Register Point= 0x%p\n", m_pMaxPoint);
//检索所有使用的结构体，如果有指针非0，表示没有释放的，打印报警信息。
//请注意，为了优化效率，这里循环的终点是m_nUseMax，不检查没有使用的部分
		for (i = 0; i < m_nUseMax; i++) {
			if (m_RegisterArray[i].m_pPoint) {
//原则上，应该帮助应用程序释放，但考虑到，这个模块是基础支撑模块。
//本析构函数执行时，进程即将关闭，因此，仅仅报警，不释放
				TONY_DEBUG("***** Memory Lost: [%p] - %s\n",
						m_RegisterArray[i].m_pPoint,
						m_RegisterArray[i].m_szInfo);
			}
		}
	}
	m_Lock.Unlock(); //解锁
}

void CMemoryRegister::RegisterCopy(STonyXiaoMemoryRegister* pDest, //目标结构体指针
		void* pPoint, //待拷贝的注册指针
		char* szInfo) //待拷贝的说明文字
		{
	pDest->m_pPoint = pPoint;
	if (szInfo) //由于szInfo 可以是null，因此需要加判断
	{ //请注意这里对SafeStrcpy 的使用，拷贝永远是安全的，程序减少很多判断，显得很简洁
		SafeStrcpy(pDest->m_szInfo, szInfo, TONY_MEMORY_BLOCK_INFO_MAX_SIZE);
	} else
		TONY_CLEAN_CHAR_BUFFER(szInfo); //如果为空，则缓冲区置为空字符串
}

//注册方法
void CMemoryRegister::Add(void* pPoint, char* szInfo) {
	int i = 0;
	m_Lock.Lock(); //加锁
	{
//完成统计功能，如果新注册的指针比最大指针大，更新最大指针
		if (pPoint > m_pMaxPoint)
			m_pMaxPoint = pPoint;
//循环遍历已经使用的数组区域，寻找因Del 导致的空区，重复使用
//注意，循环的终点是m_nUseMax，而不是数组最大单元
		for (i = 0; i < m_nUseMax; i++) {
//请注意，如果结构体保存的指针为空，判定未使用
			if (!m_RegisterArray[i].m_pPoint) {
//统计在用指针总数
				m_nPointCount++;
//调用拷贝功能函数，执行真实的拷贝动作，保存传入的信息，即注册
				RegisterCopy(&(m_RegisterArray[i]), pPoint, szInfo);
//功能完成，直接跳到最后，返回
				goto CMemoryRegister_Add_End_Process;
			}
		} //这是第二逻辑段落，即使用区域无空区，需要增补到最尾
//首先，检查是否数组越界，如果越界，报警
		if (TONY_MEMORY_REGISTER_MAX <= m_nUseMax) {
//使用Debug 模块报警，原则上，程序员如果在Debug 信息中看到这一句
//表示该程序并发的指针数超过了数组限制，可以考虑
//增加TONY_MEMORY_REGISTER_MAX 的值。
			TONY_DEBUG("***ERROR*** CMemoryRegister is full!\n");
			goto CMemoryRegister_Add_End_Process;
		} //这是讲注册内容拷贝到队列最尾的逻辑
		RegisterCopy(&(m_RegisterArray[m_nUseMax]), pPoint, szInfo);
		m_nPointCount++;
		m_nUseMax++;
	}
	CMemoryRegister_Add_End_Process: m_Lock.Unlock(); //解锁
}
//反注册函数
void CMemoryRegister::Del(void* pPoint) {
	int i = 0;
	m_Lock.Lock(); //加锁
	{
//寻找在用内存区域
		for (i = 0; i < m_nUseMax; i++) {
			if (pPoint == m_RegisterArray[i].m_pPoint) {
//统计值-1
				m_nPointCount--;
//清空逻辑，本结构体也就空闲出来，等待下次Add 重用
				m_RegisterArray[i].m_pPoint = null;
				TONY_CLEAN_CHAR_BUFFER(m_RegisterArray[i].m_szInfo);
//跳出循环，返回
				goto CMemoryRegister_Del_End_Process;
			}
		}
	}
	CMemoryRegister_Del_End_Process: m_Lock.Unlock(); //解锁
}
//修改注册指针的值
void CMemoryRegister::Modeify(void* pOld, void* pNew) {
	int i = 0;
	m_Lock.Lock();
	{
//由于要切换指针，因此需要做一次统计
		if (pOld > m_pMaxPoint)
			m_pMaxPoint = pOld;
		for (i = 0; i < m_nUseMax; i++) {
//注意，所有的检索以内存块指针作为依据，相当于哈希上的key
			if (pOld == m_RegisterArray[i].m_pPoint) {
//仅仅修订指针，不修订说明文字
				m_RegisterArray[i].m_pPoint = pNew;
				goto CMemoryRegister_Del_End_Process;
			}
		}
	} //这是一个显式的报警，如果一个指针没有找到，而应用层又来请求修改
//这表示应用程序有bug，因此，debug 提醒程序员修改bug
	TONY_DEBUG(
			"***ERROR*** CMemoryRegister::Modeify(): I can\'t found point!\n");
	CMemoryRegister_Del_End_Process: m_Lock.Unlock();
}
//信息打印函数
void CMemoryRegister::PrintInfo(void) {
	m_Lock.Lock();
	{
		TONY_XIAO_PRINTF("memory pool: %d / %d, biggest=%p\n", m_nPointCount, //在用指针总数
				m_nUseMax + 1, //注册数组使用范围
				m_pMaxPoint); //统计的最大指针
	}
	m_Lock.Unlock();
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//**************************socket 注册管理类*****************************//
//////////////////////////////////////////////////////////////////////////
//判断socket 是否合法的通用函数，这个函数设置的目的是收拢所有的判断，方便以后修改
bool SocketIsOK(Linux_Win_SOCKET nSocket) {
//某些系统的socket 判断，是以0~65535 的绝对值判断，这里实现，暂时不用
// if(0>nSocket) return false;
// if(65535<nSocket) return false;
//原则上，只要不等于Linux_Win_InvalidSocket 的其他整数，均可视为合法socket
	if (Linux_Win_InvalidSocket == nSocket)
		return false;
	return true;
}
//真实地关闭一个socket
void _Linux_Win_CloseSocket(Linux_Win_SOCKET nSocket) {
	if (!SocketIsOK(nSocket))
		return;
#ifdef WIN32
	closesocket(nSocket);
#else // not WIN32
	close(nSocket);
#endif
}
//////////////////////////////////////////////////////////////////////////////
//构造函数
CSocketRegister::CSocketRegister(CTonyLowDebug* pDebug) {
	m_pDebug = pDebug; //保存debug 对象指针
//初始化各种变量
	m_nMaxSocket = Linux_Win_InvalidSocket;
	m_nSocketUseCount = 0;
	m_nUseMax = 0;
	int i = 0;
	for (i = 0; i < TONY_MEMORY_REGISTER_MAX; i++) {
//同上，我们以一个结构体中保留的socket= Linux_Win_InvalidSocket
//作为结构体未使用的标志
		m_RegisterArray[i].m_nSocket = Linux_Win_InvalidSocket;
		TONY_CLEAN_CHAR_BUFFER(m_RegisterArray[i].m_szInfo);
	}
}

//析构函数
CSocketRegister::~CSocketRegister() {
	int i = 0;
	m_Lock.Lock();
	{
		TONY_DEBUG("CSocketRegister: Max Socket Count=%d, Max Socket=%d\n",
				m_nUseMax, m_nMaxSocket); //打印关键信息
		for (i = 0; i < m_nUseMax; i++) {
			if (SocketIsOK(m_RegisterArray[i].m_nSocket)) {
//退出时，如果发现有在用的socket，报警，并代为释放
				TONY_DEBUG("***** Socket Lost: [%d] - %s\n",
						m_RegisterArray[i].m_nSocket,
						m_RegisterArray[i].m_szInfo);
//这是释放Socket 的语句
				_Linux_Win_CloseSocket(m_RegisterArray[i].m_nSocket);
			}
		}
	}
	m_Lock.Unlock();
}

//Socket 注册管理模块的注册函数
void CSocketRegister::Add(Linux_Win_SOCKET s, char* szInfo) {
	int i = 0;
	m_Lock.Lock(); //加锁
	{
//统计行为，统计最大的socket 值供查阅
		if (!SocketIsOK(m_nMaxSocket))
			m_nMaxSocket = s;
		else if (s > m_nMaxSocket)
			m_nMaxSocket = s;
//先试图修改，遍历使用区域
		for (i = 0; i < m_nUseMax; i++) {
			if (m_RegisterArray[i].m_nSocket == s) {
				if (szInfo) //拷贝说明文字
					SafeStrcpy(m_RegisterArray[i].m_szInfo, szInfo,
							TONY_MEMORY_BLOCK_INFO_MAX_SIZE);
//注意，修改不是添加，因此，这里的socket 使用统计不增加
//m_nSocketUseCount++;
				goto CSocketRegister_Add_End_Process;
			}
		} //再试图插入
		for (i = 0; i < m_nUseMax; i++) {
			if (!SocketIsOK(m_RegisterArray[i].m_nSocket)) {
				m_RegisterArray[i].m_nSocket = s;
				if (szInfo) //拷贝说明文字
					SafeStrcpy(m_RegisterArray[i].m_szInfo, szInfo,
							TONY_MEMORY_BLOCK_INFO_MAX_SIZE);
//这是实实在在的添加到空区，因此，统计变量+1
				m_nSocketUseCount++;
				goto CSocketRegister_Add_End_Process;
			}
		} //最后无空区可以使用呢，试图追加到最后
		if (TONY_MEMORY_REGISTER_MAX > m_nUseMax) {
			m_RegisterArray[m_nUseMax].m_nSocket = s;
			if (szInfo) //拷贝说明文字
				SafeStrcpy(m_RegisterArray[m_nUseMax].m_szInfo, szInfo,
						TONY_MEMORY_BLOCK_INFO_MAX_SIZE);
//使用区域指针+1
			m_nUseMax++;
//统计变量+1
			m_nSocketUseCount++;
		} //注册区满了，报警，并发的socket 数量超限，程序员有必要扩大缓冲区
		else
			TONY_DEBUG("CSocketRegister::Add(): Pool is full!\n");
	}
	CSocketRegister_Add_End_Process: m_Lock.Unlock(); //解锁
}

//反注册函数，即将一个socket 的注册信息，从内部管理数据区删除
bool CSocketRegister::Del(Linux_Win_SOCKET s) {
	bool bRet = false;
	int i = 0;
	m_Lock.Lock(); //加锁
	{
		for (i = 0; i < m_nUseMax; i++) {
//遍历使用区，检索socket
			if (m_RegisterArray[i].m_nSocket == s) {
//注意清空动作，把socket 置为Linux_Win_InvalidSocket
//这项，下次Add 可以重复使用该空区
				m_RegisterArray[i].m_nSocket = Linux_Win_InvalidSocket;
				TONY_CLEAN_CHAR_BUFFER(m_RegisterArray[i].m_szInfo);
//修订统计变量，并发socket 数量-1
				m_nSocketUseCount--;
				bRet = true;
				goto CSocketRegister_Del_End_Process;
			}
		}
	}
	CSocketRegister_Del_End_Process: m_Lock.Unlock(); //解锁
	return bRet;
}
//内部信息打印函数
void CSocketRegister::PrintInfo(void) {
	m_Lock.Lock(); //加锁
	{
		TONY_XIAO_PRINTF("socket pool: %d / %d, biggest=%d\n",
				m_nSocketUseCount, //并发的socket 数量统计
				m_nUseMax + 1, //内存结构体数组使用量标示
				m_nMaxSocket); //统计到的最大socket
	}
	m_Lock.Unlock(); //解锁
}
//////////////////////////////////////////////////////////////////////////
//***************************內存池**************************************//
//////////////////////////////////////////////////////////////////////////
//构造函数
CTonyMemoryPoolWithLock::CTonyMemoryPoolWithLock(CTonyLowDebug* pDebug,
		bool bOpenRegisterFlag) {
	m_pDebug = pDebug; //保存debug 对象指针
	m_pMemPool = new CTonyMemoryStack(m_pDebug); //实例化内存栈对象
	m_pRegister = null; //初始化各个指针变量
	m_pSocketRegister = null;
//	m_pLockRegister = null;
	if (bOpenRegisterFlag) { //根据标志位，决定是否实例化各个注册对象
		m_pRegister = new CMemoryRegister(m_pDebug);
		m_pSocketRegister = new CSocketRegister(m_pDebug);
	} //打印内存池正确启动标志
	TONY_DEBUG("Tony.Xiao. Memory Pool Open, register flag=%d\n",
			bOpenRegisterFlag);
}
//析构函数
CTonyMemoryPoolWithLock::~CTonyMemoryPoolWithLock() {
//依次摧毁各个子对象
	if (m_pRegister) //请读者注意删除对象的标准写法
	{
		delete m_pRegister;
		m_pRegister = null;
	}
	if (m_pSocketRegister) {
		delete m_pSocketRegister;
		m_pSocketRegister = null;
	}
	if (m_pMemPool) {
		delete m_pMemPool;
		m_pMemPool = null;
	} //打印内存池正确结束标志
	TONY_DEBUG("Tony.Xiao. Memory Pool Close.\n");
}
//设置退出标志，加速内存栈的释放过程
void CTonyMemoryPoolWithLock::SetCloseFlag(bool bFlag) {
	if (m_pMemPool)
		m_pMemPool->SetCloseFlag(bFlag);
}
//Malloc 函数
void* CTonyMemoryPoolWithLock::Malloc(int nSize, char* szInfo) {
	void* pRet = null;
	if (m_pMemPool) {
		pRet = m_pMemPool->Malloc(nSize); //调用内存栈实现内存分配
		if (pRet)
			Register(pRet, szInfo); //如果指针有效，自动注册
	}
	return pRet;
}
//Free 函数
void CTonyMemoryPoolWithLock::Free(PVOID pBlock) {
	if (m_pMemPool)
		m_pMemPool->Free(pBlock); //调用内存栈实现Free
	UnRegister(pBlock); //反注册指针
}
//ReMalloc 函数
void* CTonyMemoryPoolWithLock::ReMalloc(void* pPoint, int nNewSize,
		bool bCopyOldDataFlag) {
	void* pRet = null;
	if (m_pMemPool) { //调用内存栈重定义内存区域大小
		pRet = m_pMemPool->ReMalloc(pPoint, nNewSize, bCopyOldDataFlag);
		if (m_pRegister) {
			if (pRet) //如果重分配成功，在注册对象中Modeify 新的指针
				m_pRegister->Modeify(pPoint, pRet);
			else
				//如果失败，由于老指针已经摧毁，因此需要从注册对象中反注册旧指针
				m_pRegister->Del(pPoint);
		}
	}
	return pRet;
}
//指针注册方法
void CTonyMemoryPoolWithLock::Register(void* pPoint, char* szInfo) {
	if (m_pRegister) {
		m_pRegister->Add(pPoint, szInfo);
	}
}
//指针反注册方法
void CTonyMemoryPoolWithLock::UnRegister(void* pPoint) {
	if (m_pRegister) {
		m_pRegister->Del(pPoint);
	}
}
//应用程序注册一个socket 开始管理，注意，一个socket，可能被注册多次，以修改其说明文字
//本函数是可以多次重入的。
void CTonyMemoryPoolWithLock::RegisterSocket(Linux_Win_SOCKET s, char* szInfo) {
	if (m_pSocketRegister) {
		m_pSocketRegister->Add(s, szInfo);
	}
}

//公有的关闭Socket 方法，反注册+关闭。
void CTonyMemoryPoolWithLock::CloseSocket(Linux_Win_SOCKET& s) {
	if (SocketIsOK(s)) {
		if (m_pSocketRegister) {
			if (!m_pSocketRegister->Del(s)) {
//这是一个很重要的报警，如果一个socket，经检查，没有在内部注册
//但是应用程序却call 这个方法，就表示程序内存在没有注册的socket 情况
//说明socket 的管理不完整，有遗漏，程序员有必要检查程序
//还有一种可能，这个socket 被两个程序关闭，第一次关闭时，
//其信息已经反注册，第二次就存在找不到的现象
//这说明程序员对socket 的关闭有冗余行为，虽然不会有什么问题，
//但建议最好修改。
				TONY_DEBUG(
						"CTonyMemoryPoolWithLock::CloseSocket(): \
Socket %d is not registered! but I have close it yet!\n",
						s);
			}
		} //真实地关闭socket
		_Linux_Win_CloseSocket(s);
		s = Linux_Win_InvalidSocket; //关闭完后，立即赋值
	}
}
//调用内存栈功能，打印详细跟踪信息
void CTonyMemoryPoolWithLock::PrintTree(void) {
	if (m_pMemPool)
		m_pMemPool->PrintStack();
} //打印各个子对象信息
void CTonyMemoryPoolWithLock::PrintInfo(void) {
	if (m_pSocketRegister) //打印Socket 注册对象信息
		m_pSocketRegister->PrintInfo();
	if (m_pRegister) //打印指针注册对象信息
		m_pRegister->PrintInfo();
	if (m_pMemPool) //打印内存栈对象信息
		m_pMemPool->PrintInfo();
}
