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
#include "Utils.h"
#include "BaseLib.h"

CTonyBaseLibrary::CTonyBaseLibrary(char* szAppName, char* szLogPath,
		char* szTempPath, int nTaskPoolThreadMax, bool bDebug2TTYFlag,
		_BASE_LIBRARY_PRINT_INFO_CALLBACK pPrintInfoCallback,
		void* pPrintInfoCallbackParam, _APP_INFO_OUT_CALLBACK pInfoOutCallback,
		void* pInfoOutCallbackParam) {
	m_pDebug = null; //各指针变量赋初值null
	m_pTaskPool = null; //这是预防某个初始化动作失败后
	m_pMemPool = null; //跳转，导致后续指针没有赋值
	m_pLog = null; //成为“野指针”
//保存各个路径字符串的值
	SafeStrcpy(m_szAppName, szAppName, TONY_APPLICATION_NAME_SIZE);
	SafeStrcpy(m_szLogPathName, szLogPath, TONY_APP_LOG_PATH_NAME_SIZE);
	SafeStrcpy(m_szTempPathName, szTempPath, TONY_APP_TEMP_PATH_NAME_SIZE);
//保存信息打印回调函数相关指针
	m_pPrintInfoCallback = pPrintInfoCallback;
	m_pPrintInfoCallbackParam = pPrintInfoCallbackParam;
//初始化随机数种子
	srand((unsigned int) time(NULL));
#ifdef WIN32
	{ //注意，大括号限定作用域，可以临时定义变量
		m_bSocketInitFlag=false;
		WORD wVersionRequested;
		int err;
		wVersionRequested = MAKEWORD( 2, 2 );
		err = WSAStartup( wVersionRequested, &m_wsaData );
		if ( err != 0 )
		{
			TONY_DEBUG("Socket init fail!\n");
		} else m_bSocketInitFlag=true;
	}
#else // Non-Windows
#endif
	m_pDebug = new CTonyLowDebug(m_szLogPathName, m_szAppName, bDebug2TTYFlag,
			pInfoOutCallback, pInfoOutCallbackParam);
	if (!m_pDebug)
		return;
//开始第一步信息输出，Hello World，这是笔者习惯，每个程序一启动，打印这个信息
	TONY_DEBUG("------------------------------------------\n");
	TONY_DEBUG("Hello World!\n");
	m_pMemPool = new CTonyMemoryPoolWithLock(m_pDebug);
	if (!m_pMemPool) { //如果失败，这里已经能打印Debug 信息了
		TONY_DEBUG("CTonyBaseLibrary(): m_pMemPool new fail!\n");
		return;
	}
	m_pLog = new CTonyXiaoLog(m_pDebug, m_pMemPool, m_szLogPathName, //注意，这里使用日志路径
			m_szAppName);
	if (m_pLog) { //注意，此时已经可以利用内存池的注册机制
		m_pMemPool->Register(m_pLog, "CTonyBaseLibrary::m_pLog");
	}
	m_pTaskPool = new CTonyXiaoTaskPool(this, nTaskPoolThreadMax);
	if (m_pTaskPool) { //注意，此时已经可以利用内存池的注册机制
		m_pMemPool->Register(m_pTaskPool, "CTonyBaseLibrary::m_pTaskPool");
	}
	m_pTaskRun = new CTonyTaskRun(this);
	if (m_pTaskRun) { //注意，此时已经可以利用内存池的注册机制
		m_pMemPool->Register(m_pTaskRun, "CTonyBaseLibrary::m_pTaskRun");
	}
	TimeSetNow (m_tLastPrint); //计时因子
//注意，任务回调函数的参数是本对象指针
	if (!m_pTaskRun->StartTask(InfoPrintTaskCallback, this)) { //失败则报警
		m_pLog->_XGSyslog("CTonyBaseLibrary:: start print info task fail!\n");
	} //笔者习惯，聚合工具类启动完毕标志，也是应用程序逻辑开始启动标志
	TONY_DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
}

CTonyBaseLibrary::~CTonyBaseLibrary() { //笔者习惯，这是应用程序逻辑开始退出的标志，相对于构造函数中的输出
	TONY_DEBUG("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
//这是一个技巧，退出时，笔者喜欢按照功能模块的划分，打印一些数字
//当某个模块线程出现bug，其退出逻辑很可能受到影响，导致退出时挂死
//此时只要简单观察数字打到几，就可以轻松判定是下面哪一步出现了问题，方便debug
	TONY_XIAO_PRINTF("1\n");
//设置内存池的关闭标志，内存块的Free 动作将直接释放，不再推入内存栈，加快程序运行
	m_pMemPool->SetCloseFlag();
	TONY_XIAO_PRINTF("2\n");
	TONY_XIAO_PRINTF("3\n");
	if (m_pTaskRun) //退出线程池运行体，注意其中反注册动作
	{
		m_pMemPool->UnRegister(m_pTaskRun);
		delete m_pTaskRun;
		m_pTaskRun = null;
	}
	TONY_XIAO_PRINTF("4\n");
	TONY_XIAO_PRINTF("5\n");
	if (m_pTaskPool) //退出线程池，注意其中反注册动作
	{
		m_pMemPool->UnRegister(m_pTaskPool);
		delete m_pTaskPool;
		m_pTaskPool = null;
	}
	TONY_XIAO_PRINTF("6\n");
	if (m_pLog) //退出Log 日志模块，注意其中反注册动作
	{
		m_pMemPool->UnRegister(m_pLog);
		delete m_pLog;
		m_pLog = null;
	}
	TONY_XIAO_PRINTF("7\n");
	if (m_pMemPool) //退出内存池
	{
		delete m_pMemPool;
		m_pMemPool = null;
	}
	TONY_XIAO_PRINTF("8\n");
	TONY_DEBUG("Bye World!\n"); //笔者习惯，“再见，世界”
	TONY_DEBUG("------------------------------------------\n");
	if (m_pDebug) //退出Debug 对象
	{
		delete m_pDebug;
		m_pDebug = null;
	}
	TONY_XIAO_PRINTF("9\n");
#ifdef WIN32
	if(m_bSocketInitFlag)
	{
		if ( LOBYTE( m_wsaData.wVersion ) != 2 ||
				HIBYTE( m_wsaData.wVersion ) != 2 )
		{
			WSACleanup( );
		}m_bSocketInitFlag=false;
	}
#else // Non-Windows
#endif
}
#define MAIN_LOOP_DELAY 2 //2 秒一次main loop
bool CTonyBaseLibrary::InfoPrintTaskCallback(void* pCallParam, int& nStatus) { //强制指针类型转换，获得本对象指针
	CTonyBaseLibrary* pThis = (CTonyBaseLibrary*) pCallParam;
//检测是否已经等到2 秒，否则返回
	if (TimeIsUp(pThis->m_tLastPrint, MAIN_LOOP_DELAY)) {
		TimeSetNow(pThis->m_tLastPrint); //更新当前时间
		TONY_XIAO_PRINTF("*******************************************\n");
		pThis->m_pTaskPool->PrintInfo(); //打印任务池信息
		pThis->m_pTaskRun->PrintInfo(); //打印任务池运行体信息
		pThis->m_pMemPool->PrintInfo(); //打印内存池信息
		if (pThis->m_pPrintInfoCallback) //回调应用层打印函数
			pThis->m_pPrintInfoCallback(pThis->m_pPrintInfoCallbackParam);
		TONY_XIAO_PRINTF("*******************************************\n");
		TONY_XIAO_PRINTF("\n");
	}
	return true; //返回真，永远循环
}
