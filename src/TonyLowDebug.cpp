#include "Debug.h"
#include "std.h"
#include "MutexLock.h"
#include "TonyLowDebug.h"

CTonyLowDebug::CTonyLowDebug(char* szPathName, char* szAppName,
		bool bPrint2TTYFlag, _APP_INFO_OUT_CALLBACK pInfoOutCallback,
		void* pInfoOutCallbackParam) {
	//保留回调函数指针，供业务函数调用
	m_pInfoOutCallback = pInfoOutCallback;
	m_pInfoOutCallbackParam = pInfoOutCallbackParam;
	//保留输出到屏幕的标志，供业务函数参考
	m_bPrint2TTYFlag = bPrint2TTYFlag;
	//拼接输出文件名
	if (szAppName)
		FULL_NAME(szPathName, szAppName, m_szFileName, "dbg")
	else
		//允许不输出到文件
		m_szFileName[0] = '\0';
	//先删除上次的运行痕迹，避免两次输出干扰
	DeleteAFile (m_szFileName);
	Debug2File("CTonyLowDebug: Start!\n"); //一个简单的声明，我开始工作了
}

CTonyLowDebug::~CTonyLowDebug() {
	Debug2File("CTonyLowDebug: Stop!\n");
}

char* CTonyLowDebug::GetTrueFileName(char* szBuffer) {
	char* pRet = szBuffer;
	int nLen = strlen(szBuffer);
	int i = 0;
	for (i = nLen - 1; i >= 0; i--) //逆向检索，请注意，这是老代码，不太符合无错化原则
			{
		//基本逻辑，找到右数第一个路径间隔符跳出，以此作为文件名开始点
		if ('\\' == *(szBuffer + i)) //考虑到Windows 的路径间隔符
				{
			pRet = (szBuffer + i + 1);
			break;
		}
		if ('/' == *(szBuffer + i)) //考虑到Unix 和Linux 的路径间隔符
				{
			pRet = (szBuffer + i + 1);
			break;
		}
	}
	return pRet; //返回的是找到的点，缓冲区是同一缓冲区，该返回值不需要释放
}

void CTonyLowDebug::DeleteAFile(char* szFileName) {
	remove(szFileName);
}

int CTonyLowDebug::Debug2File(char *szFormat, ...) {
	//注意，这个类设计时，还没有内存池概念，因此，动态内存申请，原则上应该避免
	//以免出现内存碎片。此处使用静态数组实现buffer，在浮动栈建立。
	//这也是为什么这个类必须声明最大输出字符串长度的原因
	char szBuf[DEBUG_BUFFER_LENGTH]; //准备输出buffer
	char szTemp[DEBUG_BUFFER_LENGTH]; //时间戳置换的中间buffer
	char szTime[DEBUG_BUFFER_LENGTH]; //时间戳的buffer
	FILE* fp = NULL; //文件指针，以C 模式工作
	int nListCount = 0;
	va_list pArgList;
	time_t t;
	struct tm *pTM = NULL;
	int nLength = 0;
	//这是构建时间戳
	time(&t);
	pTM = localtime(&t);
	nLength = SafePrintf(szTemp, DEBUG_BUFFER_LENGTH, "%s", asctime(pTM));
	szTemp[nLength - 1] = '\0';
	SafePrintf(szTime, DEBUG_BUFFER_LENGTH, "[%s] ", szTemp);
	//注意，此处开始进入加锁，以此保证跨线程调用安全
	m_Lock.Lock();
	{ //习惯性写法，以大括号和缩进清晰界定加锁区域，醒目。
	  //下面这个段落是从SafePrintf 拷贝出来的，一个逻辑的可重用性，
	  //也可以根据需要,直接拷贝代码段实现，不一定非要写成宏或函数。
		va_start(pArgList, szFormat);
		nListCount += Linux_Win_vsnprintf(szBuf + nListCount,
				DEBUG_BUFFER_LENGTH - nListCount, szFormat, pArgList);
		va_end(pArgList);
		if (nListCount > (DEBUG_BUFFER_LENGTH - 1))
			nListCount = DEBUG_BUFFER_LENGTH - 1;
		*(szBuf + nListCount) = '\0';
		//开始真实的输出
		fp = fopen(m_szFileName, "a+");
		//请注意下面逻辑，由于本函数使用了锁，因此只能有一个退出点
		//这里笔者没有使用goto，因此，必须使用if 嵌套，确保不会中间跳出
		if (fp) { //输出到文件
			nListCount = fprintf(fp, "%s%s", szTime, szBuf);
			if (m_bPrint2TTYFlag) { //根据需要输出至控制台
				TONY_PRINTF("%s%s", szTime, szBuf);
				if (m_pInfoOutCallback) { //注意此处，回调输出给需要的上层调用
					//注意，本段为后加，没有使用前文变量，目前是减少bug 率
					char szInfoOut[APP_INFO_OIT_STRING_MAX];
					SafePrintf(szInfoOut, APP_INFO_OIT_STRING_MAX, "%s%s",
							szTime, szBuf);
					m_pInfoOutCallback(szInfoOut, //注意把输出字符串传给回调
							m_pInfoOutCallbackParam); //透传的指针
				}
			}
			fclose(fp);
		} else {
			nListCount = 0;
		}
	}
	m_Lock.Unlock(); //解锁
	return nListCount; //返回输出的字节数，所有字符串构造和输出函数的习惯
}

void CTonyLowDebug::Debug2File4Bin(char* pBuffer, int nLength) {
	m_Lock.Lock();
	{ //注意加锁区域，以及对前文的代码重用。
		//TODO: 实现dbg2file4bin
		dbg2file4bin(m_szFileName, "a+", pBuffer, nLength);
		if (m_bPrint2TTYFlag)
			dbg_bin(pBuffer, nLength);
	}
	m_Lock.Unlock();
}
