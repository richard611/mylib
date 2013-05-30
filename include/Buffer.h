#ifndef __BUFFER_H_
#define __BUFFER_H_

#ifndef TONY_BUFFER_STRING_MAX
#define TONY_BUFFER_STRING_MAX (1024) //变参处理字符串的最大长度
#endif
/////////////////////////////////////////////////////////////////////////
#define __DYNAMIC_BUFF_
/////////////////////////////////////////////////////////////////////////
//******************************动态buffer类****************************//
/////////////////////////////////////////////////////////////////////////
#ifdef __DYNAMIC_BUFF_
//Buffer 类
class CTonyBuffer {
public:
//构造函数，注意传入pMemPool 内存池对象指针。
	CTonyBuffer(CTonyMemoryPoolWithLock* pMemPool);
	~CTonyBuffer();
public:
	//请注意，典型的工具类特征，所有内部变量，全部公有，方便应用层调用
	CTonyMemoryPoolWithLock* m_pMemPool; //内存池指针
	char* m_pData; //动态内存缓冲区指针
	int m_nDataLength; //内存缓冲区长度（代数据长度）
public:
////////////////////////////////////
//尺寸设置函数
	bool SetSize(int nSize); //设置新的大小
	bool InsertSpace2Head(int nAddBytes); //在前面插入空白
	bool AddSpace2Tail(int nAddBytes); //在后面插入空白
	void CutHead(int nBytes); //从前面剪切掉一段数据
	void CutTail(int nBytes); //从后面剪切掉一段数据
////////////////////////////////////
//数值转换函数
	bool SetInt(int n); //将一个整数以二进制方式拷贝到缓冲区，带网络字节排序
	int GetInt(void); //以整数方式获得缓冲区的数值
	bool SetShort(short n); //将一个短整数以二进制方式拷贝到缓冲区，带网络字节排序
	short GetShort(void); //以短整型方式获得缓冲区的数值
	bool SetChar(char n); //将一个字节以二进制方式拷贝到缓冲区
	char GetChar(void); //以字节方式获得缓冲区的数值
////////////////////////////////////
//二进制数据追加函数
			//追加数据到最后，返回新的数据长度
	int AddData(char* szData, int nDataLength);
	//插入数据到最前面，返回新的数据长度
	int InsertData2Head(char* szData, int nDataLength);
////////////////////////////////////
//二进制数据拷贝函数
	//拷贝到一块目标缓冲区，受传入的缓冲区长度限制
	int BinCopyTo(char* szBuffer, int nBufferSize);
	//从一块来源缓冲区拷贝数据到本对象中
	int BinCopyFrom(char* szData, int nDataLength);
	//从另外一个Buffer 对象拷贝数据到本对象
	int BinCopyFrom(CTonyBuffer* pBuffer);
////////////////////////////////////
//文本数据拷贝构建函数
	int StrCopyFrom(char* szString);
	int Printf(char* szFormat, ...);
////////////////////////////////////
//数据比较函数
	int memcmp(char* szData, int nDataLength);
	int strcmp(char* szString);
};
/////////////////////////////////////////////////////////////////////////
//******************************静态buffer类****************************//
/////////////////////////////////////////////////////////////////////////
#else
#ifndef TONY_SAFE_BUFFER_MAX_SIZE
#define TONY_SAFE_BUFFER_MAX_SIZE (132*1024) //暂定132k，大家可以根据实际定义
#endif
class CTonyBuffer {
public:
//由于动静态Buffer 类api 完全一样，因此，公有函数构型必须保持一致。
	CTonyBuffer(CTonyBaseLibrary* pTonyBaseLib);
	CTonyBuffer(CTonyMemoryPoolWithLock* pMemPool);
	~CTonyBuffer() {
	} //由于没有动态内存的释放任务，析构函数不做任何事
public:
////////////////////////////////////
//二进制数据拷贝函数
	int BinCopyTo(char* szBuffer, int nBufferSize);
	int BinCopyFrom(char* szData, int nDataLength);
	int BinCopyFrom(CTonyBuffer* pBuffer);
////////////////////////////////////
//文本数据拷贝构建函数
	int StrCopyFrom(char* szString);
	int Printf(char* szFormat, ...);
////////////////////////////////////
//尺寸设置函数
	//设置新的大小
	bool SetSize(int nSize);
	//在前面插入空白
	bool InsertSpace2Head(int nAddBytes);
	//在后面插入空白
	bool AddSpace2Tail(int nAddBytes);
	//从前面剪切掉一段数据
	void CutHead(int nBytes);
	//从后面剪切掉一段数据
	void CutTail(int nBytes);
////////////////////////////////////
//数值转换函数
	bool SetInt(int n);
	int GetInt(void);
	bool SetShort(short n);
	short GetShort(void);
	bool SetChar(char n);
	char GetChar(void);
////////////////////////////////////
//二进制数据追加函数
	//追加数据到最后，返回新的数据长度
	int AddData(char* szData, int nDataLength);
	//插入数据到最前面，返回新的数据长度
	int InsertData2Head(char* szData, int nDataLength);
////////////////////////////////////
//数据比较函数
	int memcmp(char* szData, int nDataLength);
	int strcmp(char* szString);
////////////////////////////////////
//服务函数
	bool IHaveData(void);
public:
	char m_pData[TONY_SAFE_BUFFER_MAX_SIZE];//请注意这里，m_pData 变为静态数组
	int m_nDataLength;
	CTonyMemoryPoolWithLock* m_pMemPool;//保留MemPool，是为了Debug 方便
};
#endif //end of __STATIC_BUFF_
/////////////////////////////////////////////////////////////////////////
//******************************pop buffer类***************************//
/////////////////////////////////////////////////////////////////////////
//PopBuffer 队列管理变量结构体
typedef struct _TONY_POP_BUFFER_HEAD_ {
	int m_nTokenCount; //内部包含的元素个数
	int m_nAllBytesCount; //使用的总字节数
} STonyPopBufferHead; //定义的结构体变量类型
//习惯性写法，定义了结构体，立即定义其长度常量
const ULONG STonyPopBufferHeadSize = sizeof(STonyPopBufferHead);
//由于PopBuffer 缓冲区的线性连续编址特性，这个队列头结构体被设计成位于缓冲区最开始的地方
//因此，队列第一个元素真实的开始点，是缓冲区开始处向后偏移STonyPopBufferHeadSize 长度
//这个宏定义出队列元素数据区开始指针（字符型指针）
#define TONY_POP_BUFFER_TOKEN_DATA_BEGIN(p) \
(((char*)p)+STonyPopBufferTokenHeadSize)
//每个Token 的头结构体
typedef struct _TONY_POP_BUFFER_TOKEN_HEAD_ {
	int m_nDataLength; //标示该Token 的数据长度
} STonyPopBufferTokenHead; //定义的结构体变量类型
//结构体长度常量
const ULONG STonyPopBufferTokenHeadSize = sizeof(STonyPopBufferTokenHead);
//如果一笔准备推入队列的数据长度为n，则该Token 的总长度可以用该宏计算
#define TONY_POP_BUFFER_TOKEN_LENGTH(n) (n+STonyPopBufferTokenHeadSize)
//如果已知一个Token 的起始指针为p，则该Token 的数据区开始处可以用该宏计算
#define TONY_POP_BUFFER_FIRST_TOKEN_BEGIN(p) \
(((char*)p)+STonyPopBufferHeadSize)
#define TONY_POP_BUFFER_FIRST_TOKEN_BEGIN(p) \
(((char*)p)+STonyPopBufferHeadSize)

//数据枚举回调函数,返回真，继续枚举，直到结束，否则直接结束循环
typedef bool (*_TONY_ENUM_DATA_CALLBACK)(char* szData, //数据指针
		int nDataLength, //数据长度
		void* pCallParam); //代传的参数指针
//这是笔者一个习惯，写出一个回调函数构型后，立即写个Demo，后续使用者可以直接拷贝使用
//static bool EnumDataCallback(char* szData,int nDataLength,void* pCallParam);
/////////////////////////////////////////////////////////////////////////////////
//基本的PopBuffer 类，本类同时兼顾粘合类和独立类两种特性
class CTonyPopBuffer {
public:
//注意，这是粘合类的特有构造函数，内部缓冲区是外部传入，
	CTonyPopBuffer(char* szBuffer, //缓冲区指针
			int nBufferSize, //缓冲区尺寸
			bool bInitFlag = true); //是否初始化标志
	~CTonyPopBuffer();
public:
//实现“后粘合”的具体函数，即实现粘合的方法
	void Set(char* szBuffer, int nBufferSize);
//清空整个数据区，仅仅是数据归零，缓冲区不释放
	void Clean(void);
//内部信息打印函数，Debug 用，相当于前文的PrintfInfo
	void PrintInside(void);
//能否正确工作的标志函数
	bool ICanWork(void);
public:
//队列最经典的功能，追加到末尾，此处兼容普通缓冲区和Buffer 类
	int AddLast(char* szData, int nDataLength);
	int AddLast(CTonyBuffer* pBuffer);
public:
//获得当前内部元素个数
	int GetTokenCount(void);
//获得当前使用的所有字节数（包含管理字节）
	int GetAllBytes(void);
//根据即将推送进队列的数据长度，判断内部剩余空间是否够用
	bool ICanSave(int nDataLength);
public:
//获得第一个元素的长度
	int GetFirstTokenLength(void);
//获取第一个元素，这是普通buffer 版本，需要应用层保证缓冲区长度足够
//用GetFirstTokenLength 可以查询第一个元素的长度
	int GetFirst(char* szBuffer, int nBufferSize);
//获得第一个元素，这是Buffer 类版本
	int GetFirst(CTonyBuffer* pBuffer);
//删除第一个元素
	bool DeleteFirst(void);
//提取并删除第一元素，就是从队列中弹出第一个元素
	int GetAndDeleteFirst(char* szBuffer, int nBufferSize);
	int GetAndDeleteFirst(CTonyBuffer* pBuffer);
public:
//枚举遍历所有数据，提交回调函数处理，并且删除数据，返回经过处理的Token 个数
	int MoveAllData(_TONY_ENUM_DATA_CALLBACK pCallBack, PVOID pCallParam);
public:
	char* m_pBuffer; //最关键的内部缓冲区指针
	int m_nBufferSize; //内部缓冲区长度
private:
//这是队列头的指针，注意，这个指针并不是动态申请的内存块，而是指向缓冲区m_pBuffer 头
	STonyPopBufferHead* m_pHead;
};

/////////////////////////////////////////////////////////////////////////
//******************************mem Queue类***************************///
/////////////////////////////////////////////////////////////////////////
//MemQueue 链表节点数据结构
typedef struct _TONY_XIAO_QUEUE_TOKEN_HEAD_ {
	int m_nDataLength; //存储的业务数据长度
	char* m_pBuffer; //指向业务数据块的指针
	struct _TONY_XIAO_QUEUE_TOKEN_HEAD_* m_pNext; //指向下一节点的指针
} STonyXiaoQueueTokenHead; //定义的新的结构体变量类型
//笔者习惯，写完结构体，立即声明其长度常量，方便后续的内存申请。
const ULONG STonyXiaoQueueTokenHeadSize = sizeof(STonyXiaoQueueTokenHead);
///////////////////////////////////////////////////////////////////////////
#ifndef TONY_CHAIN_TOKEN_MAX
#define TONY_CHAIN_TOKEN_MAX 1024
#endif
#ifndef TONY_APPLICATION_NAME_SIZE
#define TONY_APPLICATION_NAME_SIZE 50
#endif
///////////////////////动态内存队列类////////////////////////////////////////
class CTonyXiaoMemoryQueue {
private:
//由于MemQueue 使用动态内存分配，理论上是没有限制的
//但我们知道，程序中任何数据结构，都应该有边界，否则可能会造成bug
//这里使用一个内部变量，强行界定边界值，为队列最大长度做个上限
	int m_nMaxToken; //最大的Token 限制
	int m_nTokenCount; //队列中有效Token 个数
	STonyXiaoQueueTokenHead* m_pHead; //队列头指针
	STonyXiaoQueueTokenHead* m_pLast; //加速因子，队列尾指针
	CTonyLowDebug* m_pDebug; //debug 对象指针
	CTonyMemoryPoolWithLock* m_pMemPool; //内存池指针（本类依赖内存池）
	char m_szAppName[TONY_APPLICATION_NAME_SIZE];
public:
	CTonyXiaoMemoryQueue(CTonyLowDebug* pDebug, //debug 对象指针
			CTonyMemoryPoolWithLock* pMemPool, //内存池指针
			char* szAppName, //应用名，这里代队列名
			int nMaxToken = TONY_CHAIN_TOKEN_MAX); //最大Token 上限
	~CTonyXiaoMemoryQueue();
public:
	bool ICanWork(void); //这个太熟悉了吧，是否可以工作标志
	void CleanAll(void); //清除所有Token，队列清空
	int GetFirstLength(void); //获得第一个Token 数据长度
//功能和目的同PopBuffer
	int GetTokenCount(void) {
		return m_nTokenCount;
	} //获得所有Token 总数，内联
	void PrintInside(void); //遍历打印所有队列内部Token 数据
public:
	int AddLast(char* szData, //数据指针
			int nDataLength, //数据长度
			int nLimit = -1); //这是为了防止递归层度过深做的特殊限制
	int GetFirst(char* szBuffer, int nBufferSize);
	int GetFirst(CTonyBuffer* pBuffer);
	bool DeleteFirst(void);
	int GetAndDeleteFirst(char* szBuffer, int nBufferSize);
	int GetAndDeleteFirst(CTonyBuffer* pBuffer);
public:
//从前面弹出一批数据，以nBufferSize 能装下PopBuffer 的数据为准
	int PopFromFirst(char* szBuffer, int nBufferSize);
//从PopBuffer 的数据区，弹出所有数据，追加到队列末尾
	int Push2Last(char* szData, int nDataLength);
public:
	void Write2File(char* szFileName); //队列数据写入磁盘
	int ReadFromFile(char* szFileName); //队列数据从磁盘读出
private:
	void PrintAToken(STonyXiaoQueueTokenHead* pToken);
	void WriteAToken2File(STonyXiaoQueueTokenHead* pToken, FILE* fp);
	int PopFromFirst4TonyPopBuffer(CTonyPopBuffer* pPopBuffer);
	int AddLast2ThisToken(STonyXiaoQueueTokenHead* pToken, char* szData,
			int nDataLength);
	STonyXiaoQueueTokenHead* GetAToken(void);
	bool DeleteATokenAndHisNext(STonyXiaoQueueTokenHead* pToken);
	static bool PushDataCallback(char* szData, int nDataLength,
			void* pCallParam);
};
/////////////////////////////////////////////////////////////////////////
//*********************mem Queue线程安全封装类***************************///
/////////////////////////////////////////////////////////////////////////
//MemQueue 的线程安全封装类
class CTonyMemoryQueueWithLock {
public:
	CTonyMemoryQueueWithLock(CTonyLowDebug* pDebug,
			CTonyMemoryPoolWithLock* pMemPool, char* szAppName, int nMaxToken =
					TONY_CHAIN_TOKEN_MAX);
	~CTonyMemoryQueueWithLock();
public:
	bool ICanWork(void); //对应各种公有方法名
	int AddLast(char* szData, int nDataLength);
	int GetFirst(CTonyBuffer* pBuffer);
	int GetFirst(char* szBuffer, int nBufferSize);
	int GetFirstLength(void);
	int GetTokenCount(void);
	int GetAndDeleteFirst(char* szBuffer, int nBufferSize);
	int PopFromFirst(char* szBuffer, int nBufferSize);
	int Push2Last(char* szData, int nDataLength);
	void CleanAll(void);
	bool DeleteFirst(void);
	void Write2File(char* szFileName);
	int ReadFromFile(char* szFileName);
	void PrintInside(void);
private:
	CTonyMemoryPoolWithLock* m_pMemPool;
	CTonyXiaoMemoryQueue* m_pQueue; //MemQueue 的聚合对象
	CMultiReadSingleWriteLock m_Lock; //线程安全锁，为了保证效率，
//此处使用了单写多读锁
	char m_szAppName[TONY_APPLICATION_NAME_SIZE]; //保存的AppName
};
#endif
