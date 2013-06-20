#ifndef __BASE_LIB_H_
#define __BASE_LIB_H_

#ifndef TONY_APP_LOG_PATH_NAME_SIZE
#define TONY_APP_LOG_PATH_NAME_SIZE 100
#endif
#ifndef TONY_APP_TEMP_PATH_NAME_SIZE
#define TONY_APP_TEMP_PATH_NAME_SIZE 100
#endif

#include <CSocket.h>

typedef void (*_BASE_LIBRARY_PRINT_INFO_CALLBACK)(void* pCallParam);
//static void BaseLibraryPrintInfoCallback(void* pCallParam);
typedef void (*_APP_INFO_OUT_CALLBACK)(char* szInfo,void* pCallParam);
//static void ApplicationInfomationOutCallback(char* szInfo,void* pCallParam);

class CTonyBaseLibrary {
public:
	CTonyBaseLibrary(char* szAppName, //应用名
			char* szLogPath, //日志路径
			char* szTempPath, //临时文件路径
			int nTaskPoolThreadMax = DEFAULT_THREAD_MAX, //任务池最大线程数
			bool bDebug2TTYFlag = true, //Debug 输出到屏幕开关
			_BASE_LIBRARY_PRINT_INFO_CALLBACK pPrintInfoCallback = null, //info 屏幕输出回调指针
			void* pPrintInfoCallbackParam = null, //info 回调参数指针
			_APP_INFO_OUT_CALLBACK pInfoOutCallback = null, //应用程序输出回调
			void* pInfoOutCallbackParam = null); //应用程序输出回调参数指针
	~CTonyBaseLibrary(); //析构函数
public:
//应用名的备份保存
	char m_szAppName[TONY_APPLICATION_NAME_SIZE];
//日志路径
	char m_szLogPathName[TONY_APP_LOG_PATH_NAME_SIZE];
//临时文件路径
	char m_szTempPathName[TONY_APP_TEMP_PATH_NAME_SIZE];
//日志模块
	CTonyXiaoLog* m_pLog;
//内存池
	CTonyMemoryPoolWithLock* m_pMemPool;
//线程池
	CTonyXiaoTaskPool* m_pTaskPool;
//线程池的运行体
	CTonyTaskRun* m_pTaskRun;
//内核级Debug，每次运行写一个文件，覆盖上次的
	CTonyLowDebug* m_pDebug;
private:
//Info 打印任务
	static bool InfoPrintTaskCallback(void* pCallParam, int& nStatus);
	time_t m_tLastPrint;
//打印信息的回调函数
	_BASE_LIBRARY_PRINT_INFO_CALLBACK m_pPrintInfoCallback;
	void* m_pPrintInfoCallbackParam; //回调函数参数指针
};
#endif
