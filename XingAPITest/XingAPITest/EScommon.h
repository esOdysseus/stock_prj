#ifndef  ESCOMMON_H_BY_EUNSEOK_KIM
#define  ESCOMMON_H_BY_EUNSEOK_KIM

#include "Config.h"
#include <pthread.h>

#define ASSERT( input )		
#define sleep(sec)						Sleep(1000*(sec))	// for fixibility between linux and window.
//#define usleep(sec)						Sleep( sec/1000?sec/1000:1 )	// for fixibility between linux and window.
#define PTHREAD_HANDLE(handle)			(pthread_t*)(handle)

typedef void* (*pThreadFuncType)(void*);


void* ESmalloc(size_t size);

void ESfree(void* pmem);

char* ESstrdup(const char* target);

void* ESmemdup(const void* src, const size_t size);

//wchar_t ���� char ���� ����ȯ �Լ�
//char * ConvertWCtoC(wchar_t* str);
LPSTR ConvertWCtoC(LPCWSTR s);

//char ���� wchar_t ���� ����ȯ �Լ�
wchar_t* ConverCtoWC(char* str);


char* GetCloneStringData(char* data, int length);

long GetCloneLongData(char* data, int length);

void CloneValueString(char* dest, const char* src, int destBufSize);

void CloneValueString(char* dest, const char src, int destBufSize);

bool CloneValueString(char* dest, int src, int destBufSize);

#endif
