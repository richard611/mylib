#ifndef __LOWDEBUG_H_
#define __LOWDEBUG_H_

#define DEBUG_BUFFER_LENGTH 1024 //debug 缓冲区大小
#define TONY_DEBUG_FILENAME_LENGTH 256 //文件名长
#define APP_INFO_OIT_STRING_MAX 1024

//#define TONY_PRINTF printf
//#define TONY_DEBUG printf
//Windows 和Linux 下有个差别，其路径的间隔符，Linux 下符合Unix 标准，是”/”，而Windows下为”\”，这个小小的宏定义，解决这种纷争。
#ifdef WIN32
#define PATH_CHAR "\\"
#else // not WIN32
#define PATH_CHAR "/"
#endif

//注意参数的表意，从左至右依次为路径、文件名，构建的全名缓冲区，扩展名
//Windows 和Linux 下有个差别，其路径的间隔符，Linux 下符合Unix 标准，是”/”，而Windows下为”\”，这个小小的宏定义，解决这种纷争。
#ifdef WIN32
#define PATH_CHAR "\\"
#else // not WIN32
#define PATH_CHAR "/"
#endif

//注意参数的表意，从左至右依次为路径、文件名，构建的全名缓冲区，扩展名
#define FILENAME_STRING_LENGTH 256 //文件名长度统一为256 字节
#define FULL_NAME(path,name,fullname,ext_name) \
{ \
	if(strlen(path)) \
	{ \
		if(strlen(ext_name)) \
		SafePrintf(fullname, \
				FILENAME_STRING_LENGTH, \
				"%s%s%s.%s", \
				path, \
				PATH_CHAR, \
				name, \
				ext_name); \
		else \
		SafePrintf(fullname, \
				FILENAME_STRING_LENGTH, \
				"%s%s%s", \
				path, \
				PATH_CHAR,name); \
	} \
	else \
	{ \
		if(strlen(ext_name)) \
		SafePrintf(fullname, \
				FILENAME_STRING_LENGTH, \
				"%s.%s", \
				name, \
				ext_name); \
		else \
		SafePrintf(fullname, \
				FILENAME_STRING_LENGTH, \
				"%s", \
				name); \
	} \
}
#include "MutexLock.h"
//回调函数构型
typedef void (*_APP_INFO_OUT_CALLBACK)(char* szInfo, void* pCallParam);
/////////////////////////////////////////////////////////////////////////////////
class CTonyLowDebug {
public:
//静态工具函数，删除一个文件
	static void DeleteAFile(char* szFileName);
//过滤掉__FILE__前面的路径名，避免info 太长
//从后向前过滤，直到找到一个'\\'或'/'为止，
//返回该字符下一个字符，就是真实文件名起始位置
//如果找不到，则返回首字符
	static char* GetTrueFileName(char* szBuffer);
public:
//主业务函数，输出字符串到文件或控制台，变参设计，方便使用。返回字节数，不计”\0”
	int Debug2File(char *szFormat, ...);
//二进制输出一段内存区，格式参考前文的dbg4bin
	void Debug2File4Bin(char* pBuffer, int nLength);
public:
//构造函数和析构函数
	CTonyLowDebug(char* szPathName, //路径名
			char* szAppName, //文件名
			bool bPrint2TTYFlag = false, //是否打印到控制台屏幕
			_APP_INFO_OUT_CALLBACK pInfoOutCallback = null, //额外的输出回调
			void* pInfoOutCallbackParam = null); //回调函数参数
	~CTonyLowDebug();
public:
//这是一个额外加入的功能，很多时候，应用程序有自己的输出设备，
//比如Windows 的一个窗口，这里提供一个回调函数，方便应用层
//在需要的时候，抓取输出信息，打入自己的输出队列
//至于public，是因为可能某些内部模块需要看一下这个指针，同时输出
	_APP_INFO_OUT_CALLBACK m_pInfoOutCallback;
//所有的回调函数设计者，有义务帮调用者传一根指针
	void* m_pInfoOutCallbackParam;
private:
	bool m_bPrint2TTYFlag; //内部保留的控制台输出标志
	char m_szFileName[TONY_DEBUG_FILENAME_LENGTH]; //拼接好的路径名+文件名
	CMutexLock m_Lock; //线程安全锁
};

#endif
