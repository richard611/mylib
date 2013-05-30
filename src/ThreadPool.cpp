#include "std.h"
#include "Mint.h"
#include "MutexLock.h"
#include "Debug.h"
#include "TonyLowDebug.h"
#include "ThreadManager.h"
#include "Thread.h"
#include "ThreadPool.h"
#include "MemoryManager.h"
#include "Buffer.h"
#include "Log.h"
#include "BaseLib.h"

CTonyThreadPool::CTonyThreadPool(CTonyLowDebug* pDebug) {
	m_pDebug = pDebug; //保存Debug 对象指针
	TONY_DEBUG("CTonyThreadPool Start!\n"); //显示开启信息
	THREADID id; //注意，开启管理者线程的参变量
	THREAD t; //函数内变量，表示不保存
	//注意，这里面没有使用锁对象，而使用纯C 的锁结构体完成。
	MUTEXINIT (&m_RegisterLock); //初始化线程池总的锁对象
	XMI(m_bThreadContinue, true); //初始化bThreadContinue 变量
	XMI(m_nThreadPoolThreadCount, 0); //初始化nThreadCount 变量
	XMI(m_nThreadPoolIdleThreadCount, 0); //初始化空闲线程统计变量
	//初始化数组
	int i;
	for (i = 0; i < THIS_POOLTHREAD_MAX; i++) {
		//虽然管理者线程没有保存线程句柄，但考虑到应用线程以后潜在的需求，
		//在线程的任务结构体中，还是保留了线程句柄和线程ID
		m_TToken[i].m_hThread = 0;
		m_TToken[i].m_nThreadID = 0;
		//这主要是为了调试方便，为每条服务线程设置一个显式的退出码，Windows 下，
		//线程返回可以看到这个值
		m_TToken[i].m_nExitCode = THREAD_POOL_EXIT_CODE + i;
		//任务描述，回调函数指针和透传参数指针，注意，null 表示当前无任务，
		//服务线程即使启动，也是在Idle 下空转。
		m_TToken[i].m_pCallback = null;
		m_TToken[i].m_pCallParam = null;
		//这是一个很重要的描述，线程池自己的指针，原因后述。
		m_TToken[i].m_pThreadPoolObject = this;
		//初始化线程任务单元状态变量，注意，多线程安全的变量，C 模式
		XMI(m_TToken[i].m_nState, TPOOL_THREAD_STATE_NOT_RUN);
	}
	id = 0; //开启管理者线程
	t = 0;
	THREADCREATE(ThreadPoolCtrlThread, this, t, id); //注意，管理者线程的参数是this
	Sleep (OPEN_THREAD_DELAY); //强制等待管理者线程开启
}
CTonyThreadPool::~CTonyThreadPool() {
//关闭线程，这个段落比较经典，前文已多次出现，此处不再赘述
	XMS(m_bThreadContinue, false);
	while (XMG (m_nThreadPoolThreadCount)) {
		Sleep (MIN_SLEEP);
	} //等待关闭所有线程
//摧毁所有线程参数模块的状态变量
	int i;
	for (i = 0; i < THIS_POOLTHREAD_MAX; i++) {
		XME(m_TToken[i].m_nState); //只有这一个需要摧毁
	}
	XME (m_bThreadContinue); //摧毁各线程安全变量
	XME (m_nThreadPoolThreadCount);
	XME (m_nThreadPoolIdleThreadCount);
	MUTEXDESTROY (&m_RegisterLock); //摧毁基本锁
	TONY_DEBUG("CTonyThreadPool Stop!\n"); //显示退出信息
}
//寻找一个没有使用的Token
//找到,返回编号
//没有找到(全部用满了),返回-1
int CTonyThreadPool::Search4NotUseToken(void) {
	int i;
	for (i = 0; i < THIS_POOLTHREAD_MAX; i++) //遍历整个数组查找
			{
		if (TPOOL_THREAD_STATE_NOT_RUN == XMG(m_TToken[i].m_nState))
			return i; //找到，返回index
	}
	return -1; //找不到，返回-1
}
//管理者线程，注意：这里的函数声明是跨平台的线程函数定义，使用宏完成
THREADFUNCDECL(CTonyThreadPool::ThreadPoolCtrlThread,pParam)
{
	CTonyThreadPool* pThis=(CTonyThreadPool*)pParam; //老习惯，获得this 指针
//注意，此处增加线程计数器，一般说来，线程池的使用者总是长期使用
//即管理者线程总有实例化运行的机会，因此，线程计数器在其中+1，而不是放在构造函数
	XMA(pThis->m_nThreadPoolThreadCount);
	int nIdleThread=0;//空闲线程计数
	int nNotRunThread=0;//未运行线程计数
	//请注意这个死循环，参考bThreadContinue
	while(XMG(pThis->m_bThreadContinue))
	{
		//获得当前空闲的线程数
		nIdleThread=XMG(pThis->m_nThreadPoolIdleThreadCount);
		if(WHILE_THREAD_COUNT>nIdleThread)
		{
			//如果备用的空闲线程不够10,需要添加，则启动新服务线程
			//启动前，需要先找到空闲的任务块，找不到，也不启动。
			nNotRunThread=pThis->Search4NotUseToken();
			if(-1!=nNotRunThread)
			{
				//启动线程
				THREADCREATE(ThreadPoolThread,//服务者线程名
						//注意，此处把任务数据块指针作为参数传给服务者线程，
						//因此，每个服务者线程，仅能维护一个任务数据块
						&(pThis->m_TToken[nNotRunThread]),
						//注意，此处保存了服务者线程的句柄和ID
						pThis->m_TToken[nNotRunThread].m_hThread,
						pThis->m_TToken[nNotRunThread].m_nThreadID);
						//如果没有启动起来,下轮再说，此处不再报错
			}
		}Sleep(OPEN_THREAD_DELAY); //一定要等够 250ms
	}XMD(pThis->m_nThreadPoolThreadCount); //退出时，线程计数器-1
#ifdef WIN32 //按照Linux 和Windows 两种方式退出
	return THREAD_POOL_EXIT_CODE-1;
#else // not WIN32
	return null;
#endif
}
//服务线程
THREADFUNCDECL(CTonyThreadPool::ThreadPoolThread,pParam)
{
	//由于历史原因，此处有一点歧义，此处的pThis 不是线程池对象指针
	//而是任务块的指针，由于线程池开发较早，出于“尊重”原则，此处没有做更动。
	//线程池对象指针，此处表示为：pThis->m_pThreadPoolObject
	SThreadToken* pThis=(SThreadToken*)pParam;
	//刚启动，设置为Idle 状态
	XMS(pThis->m_nState,TPOOL_THREAD_STATE_IDLE);
	//线程计数器+1
	XMA(pThis->m_pThreadPoolObject->m_nThreadPoolThreadCount);
	//Idle 线程计数器+1
	XMA(pThis->m_pThreadPoolObject->m_nThreadPoolIdleThreadCount);
	//注意这个守候循环，检索了bThreadContinue 参量，以便配合最终的退出动作
	while(XMG(pThis->m_pThreadPoolObject->m_bThreadContinue))
	{
		//取任务
		switch(XMG(pThis->m_nState))
		{	case TPOOL_THREAD_STATE_NOT_RUN:
			//这个状态表示没有线程为任务块服务，但现在本线程已经启动
			//如果仍然看到这个状态,肯定出错了，表示外部程序状态设置不对
			//但这个错误不严重,自动进那个修补一下就OK
			//修补的方法就是设置为Idle 状态
			XMS(pThis->m_nState,TPOOL_THREAD_STATE_IDLE);
			//注意此处没有break
			case TPOOL_THREAD_STATE_IDLE://没有命令
			default:
			//这是正常睡眠
			break;
			case TPOOL_THREAD_STATE_BUSY://Register 下任务了
			//请注意一个细节，这里没有把Idle 计数器-1，
			//这是因为Register 函数做了这个动作，后文有详细描述
			if(pThis->m_pCallback)//检查是不是真的有任务
			{
				//将执行权交给新的任务（一次），请注意参数传递
				pThis->m_pCallback(pThis->m_pCallParam,
						pThis->m_pThreadPoolObject->m_bThreadContinue);
				//空闲线程数+1
				XMA(pThis->m_pThreadPoolObject->//----a
						m_nThreadPoolIdleThreadCount);
			}break;
		};
		//检查空闲线程总数
		if(WHILE_THREAD_COUNT<//----b
				XMG(pThis->m_pThreadPoolObject->m_nThreadPoolIdleThreadCount))
		break;//如果备用线程超出限额,则跳出死循环，退出自己
		//所有工作做完，把自己置为Idle 状态
		if(TPOOL_THREAD_STATE_IDLE!=XMG(pThis->m_nState))//----c
		XMS(pThis->m_nState,TPOOL_THREAD_STATE_IDLE);
		Sleep(DEFAULT_THREAD_SLEEP);//睡眠，等待下次任务
	} //退出流程
	  //Idle 计数器-1
	XMD(pThis->m_pThreadPoolObject->m_nThreadPoolIdleThreadCount);//----d
	//线程总计数器-1
	XMD(pThis->m_pThreadPoolObject->m_nThreadPoolThreadCount);
	//把任务区块的状态设置为正确的值（没有线程为其服务）
	XMS(pThis->m_nState,TPOOL_THREAD_STATE_NOT_RUN);
#ifdef WIN32 //两种方式退出
	return pThis->m_nExitCode;
#else // not WIN32
	return null;
#endif
}

//获得一个空闲的线程编号
//如果无空闲,则返回-1
int CTonyThreadPool::GetAIdleThread(void) {
	int nRet = -1;
	int i;
	for (i = 0; i < THIS_POOLTHREAD_MAX; i++) //遍历检索
			{ //注意，仅仅检索Idle 一个状态
		if (TPOOL_THREAD_STATE_IDLE == XMG(m_TToken[i].m_nState)) {
			nRet = i;
			break; //找到跳出
		}
	}
	return nRet; //没找到，可能返回-1
}
//注册一个新任务
//请注意,这里有临界区保护,线程安全
//注册成功,返回_THREADPOOL_OK，否则返回其他错误码
int CTonyThreadPool::ThreadPoolRegisterANewThread(_TPOOL_CALLBACK pCallback, //任务回调函数指针
		void* pParam) //透传的任务参数指针
		{
	int nRet = _THREADPOOL_PLEASE_WAIT; //返回值设置初值
	MUTEXLOCK (&m_RegisterLock); //加锁
	int nIdleThread = GetAIdleThread(); //取得Idle 的线程编号
	if (0 > nIdleThread) { //没有找到Idle 服务线程
		if (THIS_POOLTHREAD_MAX == XMG(m_nThreadPoolThreadCount)) {
			//没有空闲线程,而线程总数已经达到上限,返回OverFlow 标志
			nRet = _THREADPOOL_OVERFLOW;
		} else {
			//没有空闲线程,仅仅是还没有来得及开启,请调用者等待
			nRet = _THREADPOOL_PLEASE_WAIT;
		}
	} else {
		//找到空闲线程,添加任务
		m_TToken[nIdleThread].m_pCallback = pCallback; //这里可以看出如何将
		m_TToken[nIdleThread].m_pCallParam = pParam; //任务添加到任务区块
		XMS(m_TToken[nIdleThread].m_nState, //注意，本任务块被设置为
				TPOOL_THREAD_STATE_BUSY); //busy，因此，不会被再次
		//添加新的任务
		XMD (m_nThreadPoolIdleThreadCount); //前文所述，Idle 计数器-1
		nRet = _THREADPOOL_OK; //返回成功标志
	}
	MUTEXUNLOCK(&m_RegisterLock); //解锁
	return nRet;
}
//一定要注册成功,可以等待新的Idle 线程被管理者线程启动，除非遇到OverFlow，否则循环等待
int CTonyThreadPool::ThreadPoolRegisterANewThreadWhile(
		_TPOOL_CALLBACK pCallback, //任务回调函数指针
		void* pParam) //透传的任务参数指针
		{
	int nRet;
	while (1) //死循环等待
	{ //调用上一函数，开始注册
		nRet = ThreadPoolRegisterANewThread(pCallback, pParam);
//注册成功，或者线程池溢出，都返回
		if (_THREADPOOL_PLEASE_WAIT != nRet)
			break;
		Sleep (OPEN_THREAD_DELAY); //最多等一会,新的线程就已经建立了
	}
	return nRet;
}
int CTonyThreadPool::ThreadPoolRegTask(_TPOOL_CALLBACK pCallback, //任务回调函数指针
		void* pParam, //透传的任务参数指针
		bool bWait4Success) //是否需要等待注册成功
		{
	if (bWait4Success) //根据标志，调用不同的函数
		return ThreadPoolRegisterANewThreadWhile(pCallback, pParam);
	else
		return ThreadPoolRegisterANewThread(pCallback, pParam);
}
//////////////////////////////////////////////////////////////////
//*************************任务池类*******************************//
///////////////////////////////////////////////////////////////////
CTonyXiaoTaskPool::CTonyXiaoTaskPool(CTonyBaseLibrary* pTonyBaseLib,
		int nMaxThread) {
	m_pTonyBaseLib = pTonyBaseLib; //保存聚合工具类指针
	m_pDebug = m_pTonyBaseLib->m_pDebug; //从聚合工具类中获得debug 对象指针，
//方便后续使用
	m_nMaxThread = nMaxThread; //保存最大线程数
	m_pTonyBaseLib->m_pLog->_XGSyslog("CTonyXiaoTaskPool(): Start!\n"); //打印启动信息
	m_bThreadContinue.Set(true); //线程管理变量初始化
	m_nThreadCount.Set(0);
	m_nThreadID.Set(0); //任务ID 种子赋初值0
//实例化任务队列
	m_pTaskQueue = new CTonyMemoryQueueWithLock(m_pTonyBaseLib->m_pDebug, //传入debug 对象指针
			m_pTonyBaseLib->m_pMemPool, //传入内存池指针
			"Tony.XiaoTaskPool"); //应用名
	if (m_pTaskQueue) { //注册到内存池进行管理，预防退出时忘了释放
		TONY_REGISTER(m_pTaskQueue, "CTonyXiaoTaskPool::m_pTaskQueue");
	} //实例化线程池
	m_pThreadPool = new CTonyThreadPool(m_pTonyBaseLib->m_pDebug);
	if (m_pThreadPool) { //注册到内存池进行管理
		TONY_REGISTER(m_pThreadPool, "CTonyXiaoTaskPool::m_pThreadPool");
	}
	if (ICanWork()) //判断前面的new 动作是否完成
	{ //启动管理线程
		if (!m_pThreadPool->ThreadPoolRegTask(TaskCtrlThread, this)) { //这是日志输出，后面章节有详细介绍
			("CTonyXiaoTaskPool:: start ctrl thread %d fail!\n");
		} else
			m_nThreadCount.Add(); //如果注册成功，线程计数器+1
	}
}
CTonyXiaoTaskPool::~CTonyXiaoTaskPool() {
//以标准方式退出本对象所属所有线程
	m_bThreadContinue.Set(false);
	while (m_nThreadCount.Get()) {
		Sleep(100);
	}
	TONY_DEL_OBJ (m_pThreadPool); //删除对象的宏
	TONY_DEL_OBJ (m_pTaskQueue);
	("CTonyXiaoTaskPool(): Stop!\n"); //打印退出信息
}
bool CTonyXiaoTaskPool::ICanWork(void) { //主要检查任务队列和线程池对象是否初始化
	if (!m_pThreadPool)
		return false;
	if (!m_pTaskQueue)
		return false;
	return true;
}
void CTonyXiaoTaskPool::PrintInfo(void) { //打印当前线程数和任务书，方便程序员观察
	TONY_XIAO_PRINTF("task pool: thread count=%d, task in queue=%d\n",
			m_nThreadCount.Get(), m_pTaskQueue->GetTokenCount());
}
void CTonyXiaoTaskPool::TaskCtrlThread(void* pCallParam, //这是主程序传进来的参数指针
		MBOOL& bThreadContinue) //如果系统要退出,线程池会修改这个参数的值为false
		{
	CTonyXiaoTaskPool* pThis = (CTonyXiaoTaskPool*) pCallParam;
	int i = 0;
	for (i = 0; i < pThis->m_nMaxThread; i++) //根据最大线程数，循环启动服务线程
			{
		if (!pThis->m_pThreadPool->ThreadPoolRegTask(TaskServiceThread,
				pThis)) { //启动失败，打印报警信息，退出启动
			pThis->m_pTonyBaseLib->m_pLog->_XGSyslog(
					"CTonyXiaoTaskPool:: start service \
thread %d fail!\n", i);
			break;
		} else
			pThis->m_nThreadCount.Add(); //启动成功，线程数+1
	}
	pThis->m_nThreadCount.Dec(); //任务完成，自己退出，因此线程计数器-1
}
//传入任务数据结构STaskPoolToken 指针，
//根据任务回调情况，返回真或者假，返回假，表示该任务已经结束
bool CTonyXiaoTaskPool::TaskServiceThreadDoIt(STaskPoolToken& Task) {
	bool bCallbackRet = //执行回调函数，并获得返回值
			Task.m_pCallback(Task.m_pUserParam, Task.m_nUserStatus);
	if (!bCallbackRet)
		return bCallbackRet; //如果返回值为假，直接返回，表示任务结束
	//如果任务返回真，表示任务尚未结束，需要重新推回任务队列，等待下次运行
	bCallbackRet = RegisterATaskDoIt(&Task); //试图重新注册到任务队列
	if (!bCallbackRet) //如果注册失败，报警，表示任务丢失
	{
		TONY_DEBUG(
				"CTonyXiaoTaskPool::TaskServiceThreadDoIt(): a task need \
				continue, but add 2 queue fail! task lost!\n");
		//TONY_DEBUG_BIN((char*) &Task, STaskPoolTokenSize);
	}
	return bCallbackRet; //返回最后结果
}
void CTonyXiaoTaskPool::TaskServiceThread(void* pCallParam, //这是主程序传进来的参数指针
		MBOOL& bThreadContinue) //如果系统要退出,线程池会修改这个参数的值为false
		{
	int nQueueRet = 0;
	STaskPoolToken Task;
	char* szTask = (char*) &Task;
//获得本对象指针
	CTonyXiaoTaskPool* pThis = (CTonyXiaoTaskPool*) pCallParam;
	int nID = pThis->m_nThreadID.Add() - 1;
//这是打印本服务线程启动信号，通常注释掉不用，调试时可能使用。
//pThis->XGSyslog("CTonyXiaoTaskPool::TaskServiceThread(): %d
//	Start
//	!\n
//	",nID);
	while (XMG(bThreadContinue)) //注意本循环，标准的线程池线程守候循环
	{ //额外增加的判断，以便判断本任务池退出标志
		if (!pThis->m_bThreadContinue.Get())
			goto CTonyXiaoTaskPool_TaskServiceThread_End_Process;
//尝试从任务队列中弹出第一个任务，并执行
		nQueueRet = pThis->m_pTaskQueue->GetAndDeleteFirst(szTask,
				STaskPoolTokenSize);
		if (STaskPoolTokenSize == nQueueRet) //如果弹出成功，表示有任务
				{
			pThis->TaskServiceThreadDoIt(Task); //调用上一函数执行
		}
		Sleep (MIN_SLEEP); //习惯性睡眠
	}
	CTonyXiaoTaskPool_TaskServiceThread_End_Process: pThis->m_nThreadCount.Dec(); //退出时，线程计数器-1
//这是打印本服务线程停止信号，通常注释掉不用，调试时可能使用。
//pThis->XGSyslog("CTonyXiaoTaskPool::TaskServiceThread(): %d
//	Stop
//	!\n
//	",nID);
}
bool CTonyXiaoTaskPool::RegisterATaskDoIt(STaskPoolToken* pToken, int nLimit) {
	bool bRet = false;
	if (STaskPoolTokenSize == m_pTaskQueue->AddLast( //一次典型的队列AddLast 行为
			(char*) pToken, STaskPoolTokenSize), nLimit)
		bRet = true;
	return bRet;
}
bool CTonyXiaoTaskPool::RegisterATask(_TASKPOOL_CALLBACK pCallback, //传入回调函数指针
		void* pUserParam) //传入参数指针
		{
	STaskPoolToken Token; //准备一个结构体实例
	if (!ICanWork())
		return false;
	if (!pCallback)
		return false;
	Token.m_pCallback = pCallback; //填充结构体
	Token.m_pUserParam = pUserParam;
	Token.m_nUserStatus = 0; //注意此处，状态机置为0
	return RegisterATaskDoIt(&Token, m_nMaxThread); //调用上面的函数，完成注册
}

//////////////////////////////////////////////////////////////////
//***********************CTonyTaskRun***************************//
//////////////////////////////////////////////////////////////////
//本重载主要应对那些单任务片段的任务，即只有一个回调函数的任务，可以直接注入启动。
bool CTonyTaskRun::StartTask(_TASKPOOL_CALLBACK pCallback, void* pCallParam,
		char* szAppName) { //先实例化一个任务描述对象，调用下面的函数完成注册任务
	CTonyTaskRunInfo InfoObj;
	InfoObj.AddTask(pCallback, pCallParam); //直接将任务数据添加到任务描述
	return StartTask(&InfoObj, szAppName);
} //本重载主要应对使用任务描述对象，并完成了任务描述的应用，注入启动任务
bool CTonyTaskRun::StartTask(CTonyTaskRunInfo* pInfoObject, char* szAppName) {
	return StartTask(pInfoObject->GetInfoPoint(), szAppName);
}
//本函数利用任务描述对象内部的数据结构体，完成最终的任务启动工作
//请注意，本函数使用了远堆传参，来启动任务。远堆传参的参数，就是前文所述的结构体
//STonyTeskRunTaskCallbackParam
bool CTonyTaskRun::StartTask(STonyTaskRunInfo* pInfoStruct, char* szAppName) {
	bool bRet = false; //准备返回值
	if (!m_ThreadManager.ThreadContinue()) //如果本对象未启动，则启动
		m_ThreadManager.Open();
//请注意这里，开始准备远堆传参的参数结构体数据区，使用了内存池动态申请。
//内存池的指针是从聚合工具类中获得的。
	STonyTeskRunTaskCallbackParam* pParam =
			(STonyTeskRunTaskCallbackParam*) m_pTonyBaseLib->m_pMemPool->Malloc(
					STonyTeskRunTaskCallbackParamSize, "CTonyTaskRun::pParam");
	if (pParam) //内存申请成功，继续执行，否则返回假
	{
		pParam->m_pTonyBaseLib = m_pTonyBaseLib; //聚合工具类指针
		pParam->m_pThis = this; //本对象指针
		pParam->m_nRunIndex = 0; //状态机归0
		if (szAppName) //如果外部提供了应用名
			SafeStrcpy(pParam->m_szAppName, //拷贝应用名
					szAppName, TONY_MEMORY_BLOCK_INFO_MAX_SIZE);
		else
			//否则清空应用名
			TONY_CLEAN_CHAR_BUFFER(pParam->m_szAppName);
//注意，这里使用了任务描述的粘合类特性，粘合到参数区的任务描述数据结构上
		CTonyTaskRunInfo InfoObj(&(pParam->m_Info));
//目的是调用CopyFrom 来拷贝外部传入的任务参数
		InfoObj.CopyFrom(pInfoStruct);
//调用聚合工具类中的任务池对象，注册任务
//注意，注册是是本类提供的标准任务执行体，该任务会解析任务描述
//提供二次执行，真正执行应用层的任务调用。
		bRet = m_pTonyBaseLib->m_pTaskPool->RegisterATask(
				TonyTeskRunTaskCallback, //这是本对象的任务
				pParam); //应用层注册的任务在这里
//会被二次解析执行
		if (bRet) {
			m_ThreadManager.AddAThread(); //如果成功，线程计数器+1
			if (szAppName) //如果有应用名，打印提示
				("%s Start...\n", pParam->m_szAppName);
		}
	}
	return bRet;
}
void CTonyTaskRun::StopAll(void) {
	m_ThreadManager.CloseAll();
}
void CTonyTaskRun::PrintInfo(void) {
	TONY_XIAO_PRINTF("task run: task count=%d\n",
			m_ThreadManager.GetThreadCount()); //打印线程总数
}
bool CTonyTaskRun::TonyTeskRunTaskCallback(void* pCallParam, int& nStatus) {
	bool bCallbackRet = false; //记录用户任务片段的调用结果
	bool bGotoNextStatus = true; //跳到下一状态的标志
	STonyTeskRunTaskCallbackParam* pParam = //强制指针类型转换，获得参数指针
			(STonyTeskRunTaskCallbackParam*) pCallParam;
	if (!pParam)
		return false; //防御性设计，如果没有参数，终止
	CTonyTaskRun* pThis = pParam->m_pThis; //方便调用，获得本类指针pThis
	switch (nStatus) //这个状态机是任务池提供的
	{
	case 0: ///这是本函数的主执行代码
		if (pParam->m_Info.m_nTaskCount > pParam->m_nRunIndex) { //只要应用层任务片段没有被轮询完毕，一直在本片段执行
			bGotoNextStatus = false; //这个=false，就是不希望跳转
//这里的回调请仔细看
			bCallbackRet = //取得应用层片段的回调结果
//根据应用层状态机pParam->m_nRunIndex，调用合适的片段
					pParam->m_Info.m_CallbackArray[pParam->m_nRunIndex](
							pParam->m_Info.m_pCallParam, //这是透传的参数指针
							pParam->m_nRunIndex); //这里最关键，大家注意此处没有把
//线程池状态机nStatus 传入应用层
//代码模块，而是使用自己的状态机
//这种“欺骗”，是透明设计的关键
//根据任务池的定义，如果应用程序代码片段返回false，表示其希望跳到下一片段
//因此，这里做应用程序状态机的+1 动作
			if (!bCallbackRet)
				pParam->m_nRunIndex++;
//注意这个设计，这是非常关键的一步。当StopAll 被调用，本对象即将退出时
//本函数检测到这个结果，并不是简单把nStatus+1 做退出
//而是开始步进应用任务片段，即把应用程序的状态机pParam->m_nRunIndex 累加
//这是强制执行完应用任务片段序列的每一步。
//最终，本任务会因为应用任务执行完毕而关闭，而不是在此因为系统退出关闭
//这样保证不至于产生资源泄漏等情况。
			if (!pThis->m_ThreadManager.ThreadContinue())
				pParam->m_nRunIndex++;
		} //如果应用程序片段已经执行完毕，则本函数状态机+1，跳转到下一片段结束任务
		else
			bGotoNextStatus = true;
		break;
	default: //exit
//如果有应用名，打印退出提示
		if (0 < strlen(pParam->m_szAppName))
			pThis->m_pTonyBaseLib->m_pLog->_XGSyslog("%s Stop!\n", pParam->m_szAppName);
		pThis->m_ThreadManager.DecAThread(); //线程计数器-1
		pThis->m_pTonyBaseLib->m_pMemPool->Free(pParam); //释放参数结构体
		return false; //返回假，结束任务
	}
	if (bGotoNextStatus)
		nStatus++; //根据前文判断，调整本函数状态
	return true; //返回真，不退出任务
}
