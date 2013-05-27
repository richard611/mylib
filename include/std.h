#ifndef __STD_H_
#define __STD_H_

//通用包含
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#ifdef WIN32 //windows 下特定包含
#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include <process.h>
#include <winsock.h>
#else //Linux Unix 下特定包含
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <errno.h>
#include <termios.h>
#include <netdb.h>
#include <getopt.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif
////////////////////////////////////////////////////////////////////   
#ifdef WIN32   
    #pragma warning (disable : 4800)    
    #pragma warning (disable : 4996)    
    #pragma warning (disable : 4200)    
    #pragma warning (disable : 4244)    
    #pragma warning (disable : 4010)    
    #define Linux_Win_vsnprintf _vsnprintf   
#else // not WIN32    
    #define Linux_Win_vsnprintf vsnprintf   
#endif 
/////////////////////////////////////////////////////////////////////
#endif
#ifndef null    
    #define null 0   
#endif
