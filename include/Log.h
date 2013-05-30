#ifndef __LOG_H_
#define __LOG_H_

#define LOG_FILE_SIZE_MAX (1*1024*1024*1024) //每个日志文件最大1G
#define LOG_ITEM_LENGTH_MAX (2*1024) //单条log 最大长度2k
#define LOG_FILE_CHANGE_NAME_PRE_SECONDS (60*60) //日志文件1 小时更换一次名字
#define LOG_FILE_INFO_BUFFER_SIZE (256*1024) //日志目录最大长度(超长删除)
#define LOF_FILE_DEFAULT_HOLD 72 //一般保留3 天的数据
#define LOG_TIME_STRING_MAX 128 //时间戳字符串长度
typedef void (*_APP_INFO_OUT_CALLBACK)(char* szInfo, void* pCallParam);
//笔者的日志管理系统，由于很多系统都有自己的日志系统，此处以笔者的英文名修饰，避免重名
#define FILENAME_STRING_LENGTH 256 //文件名长度
class CTonyXiaoLog {
public:
	//静态工具函数类
	//定制时间戳字符串
	static int MakeATimeString(char* szBuffer, int nBufferSize);
public:
	//构造函数和析构函数
	CTonyXiaoLog(CTonyLowDebug* pDebug, //debug 对象指针（log 也需要debug）
			CTonyMemoryPoolWithLock* pMemPool, //内存池指针，内存队列要用
			char* szLogPath, //日志路径
			char* szAppName, //应用名（修饰日志文件）
			int nHoldFileMax = LOF_FILE_DEFAULT_HOLD, //保留多少个文件
			bool bSyslogFlag = true, //日志级别开关，true 打开，否则关闭
			bool bDebugFlag = true, bool bDebug2Flag = false, bool bDebug3Flag =
					false, bool bPrintf2ScrFlag = true); //是否打印到屏幕开关
	~CTonyXiaoLog(); //析构函数
public:
	//公有输出方法
	void _XGDebug4Bin(char* pBuffer, int nLength); //二进制输出
	int _XGSyslog(char *szFormat, ...); //Syslog 输出，变参函数
	int _XGDebug(char *szFormat, ...); //Debug 输出，变参函数
	int _XGDebug2(char *szFormat, ...); //Debug2 输出，变参函数
	int _XGDebug3(char *szFormat, ...); //Debug3 输出，变参函数
public:
	//开关变量，对应构造函数的开关
	bool m_bSyslogFlag;
	bool m_bDebugFlag;
	bool m_bDebug2Flag;
	bool m_bDebug3Flag;
private:
	//内部功能函数
	int _Printf(char *szFormat, ...); //最核心的打印输出模块，变参函数
	void DeleteFirstFile(void); //删除最老的文件
	void FixFileInfo(void); //修订文件名目录队列
	void MakeFileName(void); //根据时间和文件大小，定制文件名
	void GetFileName(void); //获得当前可用的文件名
private:
	//内部私有变量区
	CMutexLock m_Lock; //线程安全锁
	char m_szFilePath[FILENAME_STRING_LENGTH]; //文件路径
	char m_szFileName[(FILENAME_STRING_LENGTH * 2)]; //文件名
	unsigned long m_nFileSize; //当前文件大小
	time_t m_tFileNameMake; //定制文件名的时间戳
	int m_nHoldFileMax; //保留文件的数量，构造函数传入
	_APP_INFO_OUT_CALLBACK m_pInfoOutCallback; //应用程序拦截输出回调函数
	void* m_pInfoOutCallbackParam; //透传的回调函数指针
	bool m_bPrintf2ScrFlag; //是否打印到屏幕的开关
	char m_szFileInfoName[FILENAME_STRING_LENGTH]; //文件名
	CTonyLowDebug* m_pDebug; //Debug 对象指针
	CTonyMemoryPoolWithLock* m_pMemPool; //内存池对象指针
	CTonyXiaoMemoryQueue* m_pFileInfoQueue; //文件名队列
};
#endif
