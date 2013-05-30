#ifndef __THREAD_POOL_H_
#define __THREAD_POOL_H_

//线程池回调函数指针
//每个回调函数有一次运行权
//运行结束,线程并不退出,进入IDLE 状态
typedef void (*_TPOOL_CALLBACK)(void* pCallParam, //这是主程序传进来的参数指针
		MBOOL& bThreadContinue); //如果系统要退出,线程池会修改这个参数的值为false
//习惯写法，写出一个回调函数构型，立即给出示例，方便以后的应用程序拷贝使用
//static void ThreadPoolCallback(void* pCallParam,MBOOL& bThreadContinue);
#ifndef TONY_DEBUG
#define TONY_DEBUG m_pDebug->Debug2File
#endif
//////////////////////////////////////////////////////////////////////////
#define OPEN_THREAD_DELAY 250 //新开线程的延迟ms 数,Windows 和Linux 系统,
//开线程太快的话,会出现死线程
//必须有一个延迟,经验参数建议>200ms
#define WHILE_THREAD_COUNT 10 //最多n 条线程空闲等待任务（经验参数：10）
//这不是硬性的,如果有线程已经IDLE,
//可能会比这个多,但随后会动态调整
#define DEFAULT_THREAD_SLEEP 500 //通常建议的线程睡眠参数
#define THREAD_POOL_EXIT_CODE 10000 //线程池的线程,从开始设置退出码
//总线程数上限
#ifdef _ARM_ //ARM 嵌入式系统
#define THIS_POOLTHREAD_MAX 30 //嵌入式系统下最大30 条线程
#else //PC 机
#ifdef WIN32
#define THIS_POOLTHREAD_MAX 2000 //Windows 下最大2000 条线程
#else // not WIN32
#define THIS_POOLTHREAD_MAX 300 //Linux 下最大300 条线程
#endif //WIN32
#endif
//////////////////////////////////////////////////////////////
#define TPOOL_THREAD_STATE_NOT_RUN 0 //线程池线程状态,线程未运行
#define TPOOL_THREAD_STATE_IDLE 1 //线程池线程状态,线程运行,空闲
#define TPOOL_THREAD_STATE_BUSY 2 //线程池线程状态,线程运行,有任务
//////////////////////////////////////////////////////////////
#define _THREADPOOL_CAN_NOT_USE -2 //线程池未初始化，无法工作
#define _THREADPOOL_OVERFLOW -1 //线程池溢出标志,无法注册
#define _THREADPOOL_PLEASE_WAIT 0 //线程池目前没有备用线程,请等待
#define _THREADPOOL_OK 1 //线程池注册成功
//////////////////////////////////////////////////////////////
class CTonyThreadPool;
typedef struct _THREAD_TOKEN_ {
	int m_nExitCode; //返回值,也是本线程在整个线程池的编号
	MINT m_nState; //线程状态机
	THREAD m_hThread; //线程句柄
	THREADID m_nThreadID; //线程ID
	_TPOOL_CALLBACK m_pCallback; //回调函数
	void* m_pCallParam; //回调函数参数
	CTonyThreadPool* m_pThreadPoolObject; //指向线程池对象的指针
} SThreadToken; //定义的变量类型
const unsigned long SThreadTokenSize = sizeof(SThreadToken); //结构体长度常量

//////////////////////////////////////////////////////////////////
//*************************线程池类*******************************//
///////////////////////////////////////////////////////////////////
//线程池类
class CTonyThreadPool {
public:
	CTonyThreadPool(CTonyLowDebug* pDebug); //需要传入Debug 对象指针
	~CTonyThreadPool();
public:
//注册一个新线程，返回状态值，状态值后文有叙述
	int ThreadPoolRegTask(_TPOOL_CALLBACK pCallback, //回调函数指针
			void* pParam, //代传的参数指针
			bool bWait4Success = true); //是否等待注册成功才返回
	bool TPAllThreadIsIdle(void); //检查所有线程是否空闲
	bool ThreadPoolIsContinue(void); //检查线程池运行状态
private:
//这里是真实的操作系统线程函数，其构型后文叙述
	static THREADFUNCDECL(ThreadPoolThread,pParam); //线程池服务线程
	static THREADFUNCDECL(ThreadPoolCtrlThread,pParam); //线程池管理线程
private:
//内部函数，检索没有使用的Token
	int Search4NotUseToken(void);
//内部函数，获得一个空闲线程
	int GetAIdleThread(void);
//这是完成具体注册动作的内部函数，后文详细叙述
	int ThreadPoolRegisterANewThread(_TPOOL_CALLBACK pCallback, void* pParam);
	int ThreadPoolRegisterANewThreadWhile(_TPOOL_CALLBACK pCallback,
			void* pParam);
private:
	SThreadToken m_TToken[THIS_POOLTHREAD_MAX]; //线程池任务参数静态数组
//这两个参数就是线程池安全退出控制参数，int 型的计数器+bool 型的线程持续标志
	MBOOL m_bThreadContinue; //所有Thread 继续标志
	MINT m_nThreadPoolThreadCount; //Thread 计数器
//统计变量
	MINT m_nThreadPoolIdleThreadCount; //空闲的线程数量
//线程池没有使用前述的C++锁类，而是直接使用C 的纯锁结构体完成
	MUTEX m_RegisterLock; //线程注册临界区
	CTonyLowDebug* m_pDebug; //Debug 对象指针
};
//////////////////////////////////////////////////////////////////
//*************************任务池类*******************************//
///////////////////////////////////////////////////////////////////
typedef void (*_TPOOL_CALLBACK)(void* pCallParam, //这是主程序透传进来的参数指针
		MBOOL& bThreadContinue); //如果系统要退出,线程池会修改这个参数的值为false
typedef bool (*_TASKPOOL_CALLBACK)(void* pCallParam, //代传参数指针
		int& nStatus); //程序控制状态机
#ifndef TASK_POOL_TOKEN_MAX
//最多同时并发任务数
#define TASK_POOL_TOKEN_MAX (16*1024)
#endif
#ifndef DEFAULT_THREAD_MAX
//默认线程数
#define DEFAULT_THREAD_MAX (30)
#endif
//任务池核心数据结构
typedef struct _TASK_POOL_TOKEN_ {
	_TASKPOOL_CALLBACK m_pCallback; //回调函数指针
	void* m_pUserParam; //替用户传递的回调函数参数，可以为null
	int m_nUserStatus; //代替用户传递的一个状态值，初始值默认是0
} STaskPoolToken;
//数据结构长度
const int STaskPoolTokenSize = sizeof(STaskPoolToken);
////////////////////////////////////////////////////////////////////////////
//将一根指针注册到内存池，内存池默认在聚合工具类中
#define TONY_REGISTER(pPoint,szInfo) \
m_pTonyBaseLib->m_pMemPool->Register(pPoint,szInfo)
//将一根指针从内存池反注册，内存池默认在聚合工具类中
#define TONY_UNREGISTER(pPoint) \
m_pTonyBaseLib->m_pMemPool->UnRegister(pPoint)
//将一根对象指针先执行反注册，再摧毁，最后置空
#define TONY_DEL_OBJ(p) \
if(p){TONY_UNREGISTER(p);delete p;p=null;}
//////////////////////////////////////////////////////////////////////////
//依赖类
class CTonyBaseLibrary;
class CTonyThreadPool;
class CTonyMemoryQueueWithLock;
class CTonyMemoryPoolWithLock;
class CTonyLowDebug;

//////////////////////////////任务池类///////////////////////////////////////
class CTonyXiaoTaskPool {
public:
	CTonyXiaoTaskPool(CTonyBaseLibrary* pTonyBaseLib, //依赖的工具聚合类指针
			int nMaxThread = DEFAULT_THREAD_MAX); //最大线程数
	~CTonyXiaoTaskPool();
public:
	bool ICanWork(void); //防御性设计，可运行标志
	void PrintInfo(void); //内容打印，Debug 用
public:
//注册一个新任务，返回TaskID
	bool RegisterATask(_TASKPOOL_CALLBACK pCallback, //回调函数指针
			void* pUserParam = null); //回调函数参数
private:
//真实的内部注册函数
	bool RegisterATaskDoIt(STaskPoolToken* pToken, int nLimit = -1);
//服务者线程
	bool TaskServiceThreadDoIt(STaskPoolToken& Task);
	static void TaskServiceThread(void* pCallParam, MBOOL& bThreadContinue);
//管理者线程
	static void TaskCtrlThread(void* pCallParam, MBOOL& bThreadContinue);
private:
	CMRSWbool m_bThreadContinue; //任务池自带线程管理变量
	CMRSWint m_nThreadCount; //可以自行退出所属线程
	CTonyMemoryQueueWithLock* m_pTaskQueue; //核心任务队列
	CTonyThreadPool* m_pThreadPool; //线程池指针
private:
	int m_nMaxThread; //最大任务数的保存变量
	CMRSWint m_nThreadID; //任务ID 种子
	CTonyLowDebug* m_pDebug; //debug 对象指针
	CTonyBaseLibrary* m_pTonyBaseLib; //聚合工具类指针
};

//////////////////////////////////////////////////////////////////
//************************任务描述工具类***************************//
///////////////////////////////////////////////////////////////////
#ifndef TONY_MEMORY_BLOCK_INFO_MAX_SIZE
#define TONY_MEMORY_BLOCK_INFO_MAX_SIZE 124
#endif
#define TONY_TASK_RUN_MAX_TASK 16 //最多步动作
typedef struct _TONY_TASK_RUN_INFO_ {
	int m_nTaskCount; //总共多少步骤
	void* m_pCallParam; //共用的回调函数参数
//动作回调函数数组
	_TASKPOOL_CALLBACK m_CallbackArray[TONY_TASK_RUN_MAX_TASK];
} STonyTaskRunInfo;
const ULONG STonyTaskRunInfoSize = sizeof(STonyTaskRunInfo);
class CTonyTaskRun;
typedef struct _TonyTeskRunTaskCallback_Param_ {
	STonyTaskRunInfo m_Info; //任务描述结构体
	CTonyBaseLibrary* m_pTonyBaseLib; //基本聚合工具类指针
	CTonyTaskRun* m_pThis; //任务运行体对象指针
	int m_nRunIndex; //当前执行的步距
	char m_szAppName[TONY_MEMORY_BLOCK_INFO_MAX_SIZE]; //应用名
} STonyTeskRunTaskCallbackParam;
const ULONG STonyTeskRunTaskCallbackParamSize = //结构体长度常量
		sizeof(STonyTeskRunTaskCallbackParam);
////////////////////////////////////////////////////////////////////////////
class CTonyTaskRunInfo {
public:
	STonyTaskRunInfo m_Info; //作为工具类的数据区实体，保存数据
	STonyTaskRunInfo* m_pInfo; //作为粘合类的数据区指针，指向外部数据
private:
	static void Init(STonyTaskRunInfo* pInfo) //初始化动作
			{
		pInfo->m_nTaskCount = 0; //动作计数器归零
		pInfo->m_pCallParam = null; //参数指针清空
		int i = 0;
		for (i = 0; i < TONY_TASK_RUN_MAX_TASK; i++) //清空16 个回调函数指针
			pInfo->m_CallbackArray[i] = null;
	}
public:
	STonyTaskRunInfo* GetInfoPoint(void) {
		if (m_pInfo) //优先返回m_pInfo
			return m_pInfo;
		else
			return &m_Info; //否则返回m_Info 的地址
	}
public:
//这是使用STonyTeskRunTaskCallbackParam 结构传参，实现粘合的构造函数
//使用m_pInfo 指针，指针指向STonyTeskRunTaskCallbackParam 中的m_Info
	CTonyTaskRunInfo(STonyTeskRunTaskCallbackParam* pParam) {
		m_pInfo = &(pParam->m_Info);
		Init(m_pInfo); //初始化
	} //这是直接粘合到外部的一个STonyTaskRunInfo 结构体实例的重载构造函数
//使用m_pInfo 指针，指针指向传入的结构体指针
	CTonyTaskRunInfo(STonyTaskRunInfo* pInfo) {
		m_pInfo = pInfo;
		Init(m_pInfo); //初始化
	} //这是以工具类启动，不使用m_pInfo，而使用本类自带的数据区
	CTonyTaskRunInfo() {
		m_pInfo = null;
		Init(&m_Info); //初始化
	}
	~
	CTonyTaskRunInfo() {
	} //析构函数不摧毁数据
public:
	void SetCallbackParam(void* pCallParam) {
		if (m_pInfo) //注意先后顺序，优先使用m_pInfo
			m_pInfo->m_pCallParam = pCallParam;
		else
			m_Info.m_pCallParam = pCallParam;
	}
public:
//这是添加一个回调函数，连同其参数的方法，调用了下面的单独回调函数添加方法
	bool AddTask(_TASKPOOL_CALLBACK pCallback, void* pCallParam) {
		if (pCallParam)
			SetCallbackParam(pCallParam); //有参数，则设置参数
		return AddTask(pCallback); //调用下一函数，处理回调指针
	}
	bool AddTask(_TASKPOOL_CALLBACK pCallback) {
		if (m_pInfo) //优先使用m_pInfo
		{ //检查回调函数总数是否超限，是则返回false，拒绝添加
			if (TONY_TASK_RUN_MAX_TASK <= m_pInfo->m_nTaskCount)
				return false;
//添加到数组末尾
			m_pInfo->m_CallbackArray[m_pInfo->m_nTaskCount] = pCallback;
//数组计数器+1
			m_pInfo->m_nTaskCount++;
			return true;
		} else //无m_pInfo 可用，则使用m_Info
		{ //检查回调函数总数是否超限，是则返回false，拒绝添加
			if (TONY_TASK_RUN_MAX_TASK <= m_Info.m_nTaskCount)
				return false;
//添加到数组末尾
			m_Info.m_CallbackArray[m_Info.m_nTaskCount] = pCallback;
//数组计数器+1
			m_Info.m_nTaskCount++;
			return true;
		}
	}
public:
	void CopyFrom(STonyTaskRunInfo* pInfo) {
		char* pMyInfo = null; //查找有效的拷贝目标对象
		if (m_pInfo)
			pMyInfo = (char*) m_pInfo;
		else
			pMyInfo = (char*) &m_Info;
//二进制格式拷贝
		memcpy(pMyInfo, (char*) pInfo, STonyTaskRunInfoSize);
	}
};
//////////////////////////CTonyTaskRun////////////////////////////////////
class CTonyBaseLibrary;
class CTonyTaskRun {
public:
//构造函数很简单，就是保存聚合工具类指针备用即可。
	CTonyTaskRun(CTonyBaseLibrary* pTonyBaseLib) {
		m_pTonyBaseLib = pTonyBaseLib;
	}
//析构函数也是简单关闭所有任务。
	~CTonyTaskRun() {
		StopAll();
	}
public:
//启动一个任务，这里有多个重载，方便调用
	bool StartTask(_TASKPOOL_CALLBACK pCallback, //任务片段回调函数
			void* pCallParam = null, //回调函数参数指针
			char* szAppName = null); //应用名（可以为空）
//利用Info 描述启动多次任务
	bool StartTask(STonyTaskRunInfo* pInfoStruct, //任务描述核心数据结构体
			char* szAppName = null); //应用名（可以为空）
	bool StartTask(CTonyTaskRunInfo* pInfoObject, //任务描述对象指针
			char* szAppName = null); //应用名（可以为空）
//停止所有任务，退出时用，注意，这里是温和退出，每个任务的每个片段都将得到执行
	void StopAll(void);
//工具函数，判断是否在运行中
	bool IsRunning(void) {
		return m_ThreadManager.ThreadContinue();
	}
//工具函数，获得线程总数计数
	int GetThreadCount(void) {
		return m_ThreadManager.GetThreadCount();
	}
//工具函数，打印内部信息，协助debug 或性能观测
	void PrintInfo(void);
private:
//最核心的任务执行回调函数，这实际上是任务池的一个任务回调
//但其内部逻辑实现了代码片段到完整任务的转换
	static bool TonyTeskRunTaskCallback(void* pCallParam, int& nStatus);
//这里使用了线程控制锁简化操作
	CThreadManager m_ThreadManager;
//保存的聚合工具类指针
	CTonyBaseLibrary* m_pTonyBaseLib;
};

#endif
