#ifndef __DEBUG_H_
#define __DEBUG_H_

#ifdef WIN32
#define Linux_Win_vsnprintf _vsnprintf
#else // not WIN32
#define Linux_Win_vsnprintf vsnprintf
#endif
//CON_DEBUG控制是否输出到屏幕
#define CON_DEBUG 1
#ifdef CON_DEBUG
#define CON_PRINTF printf
#else
#define CON_PRINTF /\
/printf
#endif//end of WIN32
#define TONY_FORMAT(nPrintLength,szBuf,nBufferSize,szFormat) \
{ \
    va_list pArgList; \
    va_start (pArgList,szFormat); \
    nPrintLength+=Linux_Win_vsnprintf(szBuf+nPrintLength, \
        nBufferSize-nPrintLength,szFormat,pArgList); \
    va_end(pArgList); \
    if(nPrintLength>(nBufferSize-1)) nPrintLength=nBufferSize-1; \
    *(szBuf+nPrintLength)='\0'; \
} 

#define TONY_FORMAT_WITH_TIMESTAMP(nPrintLength,szBuf,nBufferSize,szFormat) \
{ \
    time_t t; \
    struct tm *pTM=NULL; \
    time(&t); \
    pTM = localtime(&t); \
    nPrintLength+=SafePrintf(szBuf,nBufferSize,"[%s",asctime(pTM)); \
    szBuf[nPrintLength-1]='\0'; \
    nPrintLength--; \
    nPrintLength+=SafePrintf(szBuf+nPrintLength,nBufferSize-nPrintLength,"] "); \
    TONY_FORMAT(nPrintLength,szBuf,nBufferSize,szFormat); \
}
///////////////////////////////////////////////////////////////////////
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" { //纯C 输出开始
#endif
//安全的字符串拷贝函数
//参数按顺序排：目的地址，源地址，拷贝的最大字节数
extern void SafeStrcpy(char *pDest, char *pSource, int nCount);
//安全的变参输出函数
//szBuf: 用户指定的输出缓冲区
//nMaxLength：用户指定的输出缓冲区尺寸
//szFormat：格式化输出字符串（变参，可以是多个）
//返回输出的字符总数（strlen 的长度，不包括最后的’\0’）
extern int SafePrintf(char* szBuf, int nMaxLength, char *szFormat, ...);

extern int SafePrintfWithTimestamp(char* szBuf,int nBufSize,char* szFormat, ...);

#define TONY_LINE_MAX 1024      //最大一行输出的字符数
extern int TonyPrintf(bool bWithTimestamp, char* szFormat, ...);      //格式化字符串   

extern int TONY_PRINTF(char* szFormat, ...);      //格式化字符串  
extern int TONY_XIAO_PRINTF(char* szFormat, ...);      //格式化字符串  
//向指定的缓冲区输出一个时间戳字符串
//szBuf: 用户指定的输出缓冲区
//nMaxLength：用户指定的输出缓冲区尺寸
//返回输出的字符总数（strlen 的长度，不包括最后的’\0’）
extern int GetATimeStamp(char* szBuf, int nMaxLength);

//向指定的缓冲区输出一个时间戳字符串
// szFileName: 用户指定的输出文件
// szMode：常见的文件打开方式描述字符串，一般建议”a+”
//返回输出的字符总数（strlen 的长度，不包括最后的’\0’）
#define DEBUG_BUFFER_LENGTH 1024
extern int dbg2file(char* szFileName, char* szMode, char *szFormat, ...);

//输出格式
//0000 - 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 xxxxxxxxxxxxxxxx
//以ASCII 方式显示数据内容
extern int dbg_bin_ascii(char* pPrintBuffer, char* pBuffer, int nLength);
//以十六进制方式显示指定数据区内容。
extern int dbg_bin_hex(char* pPrintBuffer, char* pBuffer, int nLength);
//****************这是主入口函数
//以16 字节为一行，格式化输出二进制数据内容。
extern void dbg_bin(char* pBuffer, int nLength);

//****************这是主入口函数
//以16 字节为一行，格式化输出二进制数据内容。
extern void dbg2file4bin(char* szFileName, char* szMode, char* pBuffer,
		int nLength);

#if defined(__cplusplus) || defined(c_plusplus)
} //纯C 输出结束
#endif /*defined(__cplusplus) || defined(c_plusplus)*/

#endif//end of __DEBUG_H_
