#ifndef __MEMORYSTACK_H_
#define __MEMORYSTACK_H_

#ifndef ULONG
#define ULONG unsigned long
#endif

#include "MrswLock.h"
#include "Socket.h"
#include "TonyLowDebug.h"

typedef void* PVOID;

typedef struct _TONY_MEM_BLOCK_HEAD_ {
	ULONG m_ulBlockSize; //内存块的尺寸
	struct _TONY_MEM_BLOCK_HEAD_* m_pNext; //指向下一链表元素的指针
} STonyMemoryBlockHead;
//本结构体的长度，经过计算，恒定为8Bytes
const ULONG STonyMemoryBlockHeadSize = sizeof(STonyMemoryBlockHead);

//根据一个应用程序数据块的长度，计算一个内存块的真实大小，即n+8
#define TONY_MEM_BLOCK_SIZE(nDataLength) \
(nDataLength+STonyMemoryBlockHeadSize)
//根据向系统申请的内存块，计算其应用程序数据内存的真实大小，即n-8
#define TONY_MEM_BLOCK_DATA_SIZE(nBlockSize) \
(nBlockSize-STonyMemoryBlockHeadSize)
//根据应用程序释放的指针，逆求真实的内存块指针，即p0=p1-8
#define TONY_MEM_BLOCK_HEAD(pData) \
((STonyMemoryBlockHead*)(((char*)pData)-STonyMemoryBlockHeadSize))
//根据一个内存块的真实指针，求数据内存块的指针，即p1=p0+8
#define TONY_MEM_BLOCK_DATA(pHead) \
(((char*)pHead)+STonyMemoryBlockHeadSize)
//最小内存块长度，16 Bytes，由于我们管理占用8 Bytes，这个最小长度不能再小了，
//否则无意义，即使这样，我们最小的内存块，能分配给应用程序使用的，仅有8 Bytes。
#define TONY_XIAO_MEMORY_STACK_BLOCK_MIN 16
//这是管理的最大内存块长度，1M，如前文表中所示，超过此限制，内存池停止服务
//改为直接向系统申请和释放。
#define TONY_XIAO_MEMORY_STACK_MAX_SAVE_BLOCK_SIZE (1*1024*1024)
////////需要完善与注意//////////////TODO
#define TONY_DEBUG m_pDebug->Debug2File
/////////////////////////////////////////////////////////////////////
//************************内存栈基本单元*****************************///
/////////////////////////////////////////////////////////////////////
//内存栈最基本的管理单元，笔者习惯以Token 定名单位，单元，元素等结构，仅仅是习惯而已。
//本类使用了一定的递归逻辑
class CTonyMemoryStackToken {
public:
	//构造函数和析构函数，注意，此处要求输入本内存块管理的基本内存大小，如64 Bytes
	//以及Debug 对象的指针，本模块需要利用Debug 进行中间的输出。
	CTonyMemoryStackToken(int nBlockSize, CTonyLowDebug* pDebug);
	~CTonyMemoryStackToken();
public:
	//基本的申请函数Malloc，返回成功申请的内存块指针，注意，是p1，不是p0
	//注意其中的统计设计，这是为了Debug 和观察方便
	//另外，请注意，统计变量是传址操作，因此，可以在函数内部修改，回传信息
	// nAllBlockCount- nMemoryUse=内部的空闲内存块数量
	void* Malloc(int nSize, //应用程序申请的内存尺寸
			CMRSWint& nAllBlockCount, //统计变量，返回管理过的内存块总数
			CMRSWint& nMemoryUse); //统计变量，返回被应用程序使用的内存块数量
	//释放函数，此处的bCloseFlag 是一个特殊优化，即在程序退出时
	//释放的内存不会再返回内存栈，直接被free 掉，以加快程序退出速度。
	//注意，此处应用程序传进来的是p1，内部需要逆向计算出p0 实现操作。
	bool Free(void* pPoint, bool bCloseFlag);
	//这是纯粹的性能观察函数，这也是笔者开发底层模块的一个习惯
	//底层模块，由于很少有UI 交互的机会，因此，如果内部有什么问题，程序员很难观察到
	//由此可能导致bug，因此，一般对底层模块，笔者习惯性做一个打印函数
	//打印一些特殊信息，帮助程序员观察性能或bug。
	void PrintStack(void);
private:
	//系统退出时，递归销毁所有内存块的函数。
	//笔者一般习惯以Brother（兄弟）或Next（下一个）来定名左枝，
	//以Son（儿子）来定名右枝
	void DestroySon(STonyMemoryBlockHead* pSon);
private:
	//第一个儿子的指针，这就是链头了
	STonyMemoryBlockHead* m_pFirstSon;
	//这是指向兄弟节点，即左枝下一节点的指针
	CTonyMemoryStackToken* m_pNext;
	//最重要的设计，线程安全锁，注意，为了提升效率，这里使用了单写多读锁
	//根据本类定义，锁仅保护上述两个指针变量
	CMultiReadSingleWriteLock m_Lock;
	//内部保存一下本对象管理的内存块尺寸，就是构造函数传入的数据
	//供比对使用
	ULONG m_ulBlockSize;
	//性能统计变量，请注意，这些都是单写多读锁安全变量，本身是现成安全的。
	CMRSWint m_nBlockOutSide; //分配出去的内存块数量
	CMRSWint m_nBlockInSide; //内部保留的空闲内存块数量
	CTonyLowDebug* m_pDebug; //debug 对象指针
};
///////////////////////////////////////////////////////////////////////////
//*****************************内存栈************************************///
///////////////////////////////////////////////////////////////////////////
class CTonyMemoryStack {
public:
	CTonyMemoryStack(CTonyLowDebug* pDebug); //构造函数，需要传入Debug 对象
	~CTonyMemoryStack();
public:
//重定义指针指向的内存单元地址空间大小，给出旧指针，和新的空间尺寸
//如果成功，返回有效的新指针，指向重定义大小之后的新空间，失败返回null
//注意，本函数可以把旧空间的数据内容备份到新空间，但这不一定，取决与新空间大小
	void* ReMalloc(void* pPoint, //旧指针
			int nNewSize, //新的尺寸需求
			bool bCopyOldDataFlag = true); //是否备份旧数据的标志（默认“真”）
//Malloc 和Free 函数，调用内存块管理单元方法完成
	void* Malloc(int nSize);
	bool Free(void* pPoint);
//内部信息输出函数，下文会讲到其差别
	void PrintStack(void);
	void PrintInfo(void);
//作为一项优化，如果app 设置这个关闭标志，
//所有的free 动作都直接调用系统的free 完成，不再保留到stack 中。
//加快退出速度。
	void SetCloseFlag(bool bFlag = true);
	CTonyLowDebug* m_pDebug; //debug 对象，此处为了优化公开
private:
	CTonyMemoryStackToken* m_pHead; //注意，这是左枝开始的第一个节点
	CMRSWint m_pMaxPoint; //统计当前用到的最大内存指针
	CMRSWint m_nAllBlockCount; //统计所有在用的内存块
	CMRSWint m_nMemoryUse; //统计内存使用的总字节数
	CMRSWbool m_CloseFlag; //关闭标示
};
////////////////////////////////////////////////////////////////////////
//*****************************内存注册模块*****************************//
////////////////////////////////////////////////////////////////////////
//说明文字的最大长度，该长度在多个点被调用，因此使用条件编译声明
#ifndef TONY_MEMORY_BLOCK_INFO_MAX_SIZE
#define TONY_MEMORY_BLOCK_INFO_MAX_SIZE 124
#endif
#ifndef TONY_CLEAN_CHAR_BUFFER
#define TONY_CLEAN_CHAR_BUFFER(p) (*((char*)p)='\0')
#endif
//内存注册类核心数据结构
typedef struct _TONY_XIAO_MEMORY_REGISTER_ {
	void* m_pPoint; //保留的指针
	char m_szInfo[TONY_MEMORY_BLOCK_INFO_MAX_SIZE]; //说明文字
} STonyXiaoMemoryRegister; //定义的变量类型
//习惯性写法，声明了结构体后，立即定义其尺寸常量
const ULONG STonyXiaoMemoryRegisterSize = sizeof(STonyXiaoMemoryRegister);

//管理的最大指针个数，PC 平台一般建议10000，但嵌入式平台，
//要根据具体情况分析，一般不建议超过1024
#ifndef TONY_MEMORY_REGISTER_MAX
#define TONY_MEMORY_REGISTER_MAX 10000
#endif
//内存注册类
class CMemoryRegister {
public:
	CMemoryRegister(CTonyLowDebug* pDebug); //构造函数传输debug 对象指针
	~CMemoryRegister();
public:
//添加一个指针及其说明文字
	void Add(void* pPoint, char* szInfo);
//删除一个指针（该内存块被释放，失效）
	void Del(void* pPoint);
//remalloc 的时候更新指针
	void Modeify(void* pOld, void* pNew);
//打印信息函数，Debug 和观察性能用
	void PrintInfo(void);
private:
//这是一个内部工具函数，把一段指针和说明文字，拷贝到上面定义的结构体中
	void RegisterCopy(STonyXiaoMemoryRegister* pDest, void* pPoint,
			char* szInfo);

private:
	CTonyLowDebug* m_pDebug; //Debug 对象
//请注意，由于注册类不管是注册，还是反注册动作，均包含写动作
//此时，使用单写多读锁意义不大，因此使用普通锁来完成
	CMutexLock m_Lock; //线程安全锁
//请注意这里，这里使用了静态数组，主要原因是考虑到内存注册类
//是内存池的一个功能，不能再有动态内存申请，以免造成新的不稳定因素。
//不过请注意，这个数组占用栈空间，对于嵌入式设备，可能没有这么大，要关注不要越界。
	STonyXiaoMemoryRegister m_RegisterArray[TONY_MEMORY_REGISTER_MAX];
//这是数组使用标志，表示当前数组最大使用多少单元，
//注意，这不是并发指针数，随着反注册的进行，数组中可能有空区
	int m_nUseMax;
//另外一个统计，统计有史以来注册的最大指针，统计性能实用。
	void* m_pMaxPoint;
//这是统计的当前在用指针数。
	int m_nPointCount;
};

//////////////////////////////////////////////////////////////////////////
//**************************socket 注册管理类*****************************//
//////////////////////////////////////////////////////////////////////////
//核心管理数据结构
typedef struct _TONY_XIAO_SOCKET_REGISTER_ {
	Linux_Win_SOCKET m_nSocket; //保存的socket
	char m_szInfo[TONY_MEMORY_BLOCK_INFO_MAX_SIZE]; //说明文字，长度同上
} STonyXiaoSocketRegister;
//结构体尺寸常量
const ULONG STonyXiaoSocketRegisterSize = sizeof(STonyXiaoSocketRegister);
//判断socket 是否合法的通用函数，这个函数设置的目的是收拢所有的判断，方便以后修改
bool SocketIsOK(Linux_Win_SOCKET nSocket);
//真实地关闭一个socket
void _Linux_Win_CloseSocket(Linux_Win_SOCKET nSocket);

class CSocketRegister {
public:
	CSocketRegister(CTonyLowDebug* pDebug); //构造函数，传入debug 对象指针
	~CSocketRegister();
public:
	void PrintInfo(void); //信息打印函数
public:
//注册一个socket，以及其说明文字
	void Add(Linux_Win_SOCKET s, char* szInfo = null);
//反注册一个socket，如果内部没找到，返回false
	bool Del(Linux_Win_SOCKET s);
private:
	CTonyLowDebug* m_pDebug; //debug 对象指针
	CMutexLock m_Lock; //锁，这里同样使用普通锁
//保存注册信息的结构体数组，大小也同内存指针注册类
	STonyXiaoSocketRegister m_RegisterArray[TONY_MEMORY_REGISTER_MAX];
//管理变量
	int m_nUseMax;
//统计变量
	Linux_Win_SOCKET m_nMaxSocket; //注册过的最大socket，
	int m_nSocketUseCount; //在用的socket 总数
};
//////////////////////////////////////////////////////////////////////////
//***************************內存池**************************************//
//////////////////////////////////////////////////////////////////////////
//内存池类
class CTonyMemoryPoolWithLock {
public:
	//构造函数
	//考虑到某些成熟的代码模块，已经经过测试，确定没有bug，可以考虑在这里
	//关闭注册功能，这样，减少了每次内存申请和释放时，注册和反注册的逻辑，可以提升效率
	CTonyMemoryPoolWithLock(CTonyLowDebug* pDebug, //传入的Debug 对象指针
			bool bOpenRegisterFlag = true); //不开启注册功能的标志
	~CTonyMemoryPoolWithLock();
//////////////////////////////////////////////////////////
//指针管理
public:
	//注册指针，实现管理，注意需要说明文字
	void Register(void* pPoint, char* szInfo);
	void UnRegister(void* pPoint);
private:
	CMemoryRegister* m_pRegister; //指针注册管理对象
//////////////////////////////////////////////////////////
//Socket 管理
public:
	//注册Socket，实现管理，注意需要说明文字
	void RegisterSocket(Linux_Win_SOCKET s, char* szInfo = null);
	//代应用程序执行Close Socket，所有Socket 的Close 由此处完成
	void CloseSocket(Linux_Win_SOCKET& s); //关闭Socket
private:
	CSocketRegister* m_pSocketRegister; //Socket 注册管理对象
//////////////////////////////////////////////////////////
//公共管理
public:
	//作为一项优化，如果app 设置这个关闭标志，
	//所有的free 都直接free，不再保留到stack 中。
	//加快退出速度。
	void SetCloseFlag(bool bFlag = true);
	//重新分配一个指针的空间，默认拷贝原始数据到新空间。
	//考虑到大家可能会使用p=pMemPool->ReMalloc(p,1024);这种形式，为了防止内存泄露
	//如果pPoint 没有被找到,或者新指针分配失败，老指针会被free
	void* ReMalloc(void* pPoint, int nNewSize, bool bCopyOldDataFlag = true);
	//分配一个块，注意，需要一段说明文字，说明指针用途，方便注册跟踪
	void* Malloc(int nSize, char* szInfo = null);
	//释放一个块
	void Free(PVOID pBlock);
	//显示整棵内存树的内容
	void PrintTree(void);
	//关键信息显示
	void PrintInfo(void);
	void test(void);
	CTonyMemoryStack* m_pMemPool; //内存栈对象
	CTonyLowDebug* m_pDebug;
};
#endif
