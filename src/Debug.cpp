#include "Debug.h"
#include "std.h"
//安全的字符串拷贝函数
//参数按顺序排：目的地址，源地址，拷贝的最大字节数
void SafeStrcpy(char *pDest, char *pSource, int nCount) {
	int nLen = (int) strlen(pSource) + 1; //获得源字符串长度，+1 保证’\0’
	if (!pDest)
		goto SafeStrcpy_END_PROCESS;
	//保护措施，如果目标地址非法
	if (!pSource)
		goto SafeStrcpy_END_PROCESS;
	//则放弃服务，跳到结束点
	if (nLen > nCount)
		nLen = nCount; //避免读写出界的设计
	memcpy(pDest, pSource, nLen); //实施拷贝
	*(pDest + nLen - 1) = '\0'; //手工添加’\0’
	SafeStrcpy_END_PROCESS: //这是结束点
	return;
}

inline int SafePrintf(char* szBuf,int nBufSize,char* szFormat, ...)   
{   
    if(!szBuf) return 0;   
    if(!nBufSize) return 0;   
    if(!szFormat) return 0;   
    int nRet=0;   
    TONY_FORMAT(nRet,szBuf,nBufSize,szFormat);   
    return nRet;   
}

int SafePrintfWithTimestamp(char* szBuf,int nBufSize,char* szFormat, ...)   
{   
    if(!szBuf) return 0;   
    if(!nBufSize) return 0;   
    if(!szFormat) return 0;   
    int nRet=0;   
    TONY_FORMAT_WITH_TIMESTAMP(nRet,szBuf,nBufSize,szFormat);   
    return nRet;   
}


//输出到控制台   
int TonyPrintf(bool bWithTimestamp, char* szFormat, ...)      //格式化字符串   
{   
    if(!szFormat) return 0;   
    char szBuf[TONY_LINE_MAX];   
    int nLength=0;   
    if(!bWithTimestamp)            
    {   //注意，由于内部是函数型宏，if...else这个大括号必不可少   
        TONY_FORMAT(nLength,szBuf,TONY_LINE_MAX,szFormat);   
    }   //注意，由于内部是函数型宏，if...else这个大括号必不可少   
    else  
    {   //注意，由于内部是函数型宏，if...else这个大括号必不可少   
        TONY_FORMAT_WITH_TIMESTAMP(nLength,szBuf,TONY_LINE_MAX,szFormat);   
    }   //注意，由于内部是函数型宏，if...else这个大括号必不可少   
    return printf(szBuf);   
}
//输出到控制台   
int TONY_PRINTF(char* szFormat, ...)      //格式化字符串   
{   
	bool bWithTimestamp = false;
    if(!szFormat) return 0;   
    char szBuf[TONY_LINE_MAX];   
    int nLength=0;   
    if(!bWithTimestamp)            
    {   //注意，由于内部是函数型宏，if...else这个大括号必不可少   
        TONY_FORMAT(nLength,szBuf,TONY_LINE_MAX,szFormat);   
    }   //注意，由于内部是函数型宏，if...else这个大括号必不可少   
    else  
    {   //注意，由于内部是函数型宏，if...else这个大括号必不可少   
        TONY_FORMAT_WITH_TIMESTAMP(nLength,szBuf,TONY_LINE_MAX,szFormat);   
    }   //注意，由于内部是函数型宏，if...else这个大括号必不可少   
    return printf(szBuf);   
}
//输出到控制台   
int TONY_XIAO_PRINTF(char* szFormat, ...)      //格式化字符串   
{   
	bool bWithTimestamp = true;
    if(!szFormat) return 0;   
    char szBuf[TONY_LINE_MAX];   
    int nLength=0;   
    if(!bWithTimestamp)            
    {   //注意，由于内部是函数型宏，if...else这个大括号必不可少   
        TONY_FORMAT(nLength,szBuf,TONY_LINE_MAX,szFormat);   
    }   //注意，由于内部是函数型宏，if...else这个大括号必不可少   
    else  
    {   //注意，由于内部是函数型宏，if...else这个大括号必不可少   
        TONY_FORMAT_WITH_TIMESTAMP(nLength,szBuf,TONY_LINE_MAX,szFormat);   
    }   //注意，由于内部是函数型宏，if...else这个大括号必不可少   
    return printf(szBuf);   
}
////安全的变参输出函数
////szBuf: 用户指定的输出缓冲区
////nMaxLength：用户指定的输出缓冲区尺寸
////szFormat：格式化输出字符串（变参，可以是多个）
////返回输出的字符总数（strlen 的长度，不包括最后的’\0’）
//int SafePrintf(char* szBuf, int nMaxLength, char *szFormat, ...) {
//	int nListCount = 0;
//	va_list pArgList;
//	//此处做安全性防护，防止用户输入非法的缓冲区，导致程序在此崩溃。
//	if (!szBuf)
//		goto SafePrintf_END_PROCESS;
//	//此处开启系统循环，解析每条格式化输出字符串
//	va_start(pArgList, szFormat);
//	nListCount += Linux_Win_vsnprintf(szBuf + nListCount,
//			nMaxLength - nListCount, szFormat, pArgList);
//	va_end(pArgList);
//	//实现缓冲区超限保护
//	if (nListCount > (nMaxLength - 1))
//		nListCount = nMaxLength - 1;
//	//人工添加’\0’，确保输出100%标准C 字符串
//	*(szBuf + nListCount) = '\0';
//	SafePrintf_END_PROCESS: return nListCount;
//}

//向指定的缓冲区输出一个时间戳字符串
//szBuf: 用户指定的输出缓冲区
//nMaxLength：用户指定的输出缓冲区尺寸
//返回输出的字符总数（strlen 的长度，不包括最后的’\0’）
int GetATimeStamp(char* szBuf, int nMaxLength) {
	time_t t;
	struct tm *pTM = NULL;
	int nLength = 0;
	time(&t);
	pTM = localtime(&t);
	nLength = SafePrintf(szBuf, nMaxLength, "%s", asctime(pTM));
	//这里是个重要的技巧，asctime(pTM)产生的字符串最后会自动带上回车符，
	//这给后面很多的格式化输出带来不便
	//此处退格结束字符串，目的是过滤掉这个回车符。
	szBuf[nLength - 1] = '\0';
	return nLength;
}

//向指定的缓冲区输出一个时间戳字符串
// szFileName: 用户指定的输出文件
// szMode：常见的文件打开方式描述字符串，一般建议”a+”
//返回输出的字符总数（strlen 的长度，不包括最后的’\0’）
#define DEBUG_BUFFER_LENGTH 1024
int dbg2file(char* szFileName, char* szMode, char *szFormat, ...) {
	//前半段和SafePrintf 几乎一模一样
	char szBuf[DEBUG_BUFFER_LENGTH];
	char szTime[256];
	int nListCount = 0;
	va_list pArgList;
	va_start(pArgList, szFormat);
	nListCount += Linux_Win_vsnprintf(szBuf + nListCount,
			DEBUG_BUFFER_LENGTH - nListCount, szFormat, pArgList);
	va_end(pArgList);
	if (nListCount > (DEBUG_BUFFER_LENGTH - 1))
		nListCount = DEBUG_BUFFER_LENGTH - 1;
	*(szBuf + nListCount) = '\0';
	//在此开始正式的输出到各个目标设备
	GetATimeStamp(szTime, 256);
	FILE* fp;
	fp = fopen(szFileName, szMode);
	if (fp) {
		nListCount = fprintf(fp, "[%s] %s", szTime, szBuf); //文件打印
		CON_PRINTF("[%s] %s", szTime, szBuf); //屏幕打印
		fclose(fp);
	} else
		nListCount = 0;
	return nListCount;
}

//输出格式
//0000 - 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 xxxxxxxxxxxxxxxx
//以ASCII 方式显示数据内容
int dbg_bin_ascii(char* pPrintBuffer, char* pBuffer, int nLength) {
	//内部函数，只有笔者本人的函数调用的函数，一般不写防御性设计，保持简洁。
	int i;
	int nCount = 0;
	for (i = 0; i < nLength; i++) //请关注for 的写法，笔者所有的for 都是这个模式
			{
		//ASCII 字符表中，可显示字符代码>32
		if (32 <= *(pBuffer + i)) //请关注常量写左边
			nCount += SafePrintf(pPrintBuffer + nCount, 256, "%c",
					*(pBuffer + i));
		else
			//不可显示字符以”.”代替占位，保持格式整齐，且避免出错
			nCount += SafePrintf(pPrintBuffer + nCount, 256, ".");
		//如果指定的内容不是可以显示的ASCII 字符，显示’.’
	}
	return nCount;
}
//以十六进制方式显示指定数据区内容。
int dbg_bin_hex(char* pPrintBuffer, char* pBuffer, int nLength) {
	int i = 0;
	int j = 0;
	int nCount = 0;
	//一个比较复杂的打印循环，虽然很长，但还是很简单
	for (i = 0; i < nLength; i++) {
		nCount += SafePrintf(pPrintBuffer + nCount, 256, "%02X ",
				(unsigned char) *(pBuffer + i));
		j++;
		if (4 == j) {
			j = 0;
			nCount += SafePrintf(pPrintBuffer + nCount, 256, " ");
		}
	}
	if (16 > nLength) //每行 打印16字节
			{
		for (; i < 16; i++) {
			nCount += SafePrintf(pPrintBuffer + nCount, 256, " ");
			j++;
			if (4 == j) {
				j = 0;
				nCount += SafePrintf(pPrintBuffer + nCount, 256, " ");
			}
		}
	}
	return nCount;
}

//****************这是主入口函数
//以16 字节为一行，格式化输出二进制数据内容。
void dbg_bin(char* pBuffer, int nLength) {
	int nAddr = 0;
	int nLineCount = 0;
	int nBufferCount = nLength;
	int n = 0;
	char szLine[256]; //行缓冲
	if (0 < nLength) {
		while (1) {
			n = 0;
			n += SafePrintf(szLine + n, 256 - n, "%p - ", pBuffer + nAddr);
			nLineCount = 16;
			if (nBufferCount < nLineCount)
				nLineCount = nBufferCount;
			n += dbg_bin_hex(szLine + n, pBuffer + nAddr, nLineCount);
			n += dbg_bin_ascii(szLine + n, pBuffer + nAddr, nLineCount);
			CON_PRINTF("%s\n", szLine);
			nAddr += 16;
			nBufferCount -= 16;
			if (0 >= nBufferCount)
				break;
		}
		CON_PRINTF("\n");
	} else
		CON_PRINTF("dbg_bin error length=%d\n", nLength);
}

//****************这是主入口函数
//以16 字节为一行，格式化输出二进制数据内容。
void dbg2file4bin(char* szFileName, char* szMode, char* pBuffer, int nLength) {
	int nAddr = 0;
	int nLineCount = 0;
	int nBufferCount = nLength;
	int n = 0;
	char szLine[256]; //行缓冲
	FILE* fp;
	if (0 < nLength) {
		while (1) {
			n = 0;
			n += SafePrintf(szLine + n, 256 - n, "%p - ", pBuffer + nAddr);
			nLineCount = 16;
			if (nBufferCount < nLineCount)
				nLineCount = nBufferCount;
			n += dbg_bin_hex(szLine + n, pBuffer + nAddr, nLineCount);
			n += dbg_bin_ascii(szLine + n, pBuffer + nAddr, nLineCount);
			fp = fopen(szFileName, szMode);
			if (fp) {
				fprintf(fp, "%s\n", szLine); //文件打印
				//CON_PRINTF("[%s] %s", szTime, szBuf); //屏幕打印
				fclose(fp);
			}

			nAddr += 16;
			nBufferCount -= 16;
			if (0 >= nBufferCount)
				break;
		}
		fp = fopen(szFileName, szMode);
		if (fp) {
			fprintf(fp, "\n"); //文件打印			
			fclose(fp);
		}

	} else
		CON_PRINTF("dbg_bin error length=%d\n", nLength);
}

