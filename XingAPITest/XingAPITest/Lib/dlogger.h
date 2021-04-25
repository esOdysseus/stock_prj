#ifndef DLOGGER_H_BY_EUNSEOK_KIM
#define DLOGGER_H_BY_EUNSEOK_KIM

#include <stdio.h>

#define LOG_ERROR	0
#define LOG_WARN	1
#define LOG_INFO	2
#define LOG_DEBUG	3

#define MLOG_ERROR	"E/"
#define MLOG_WARN	"W/"
#define MLOG_INFO	"I/"
#define MLOG_DEBUG	"D/"


#ifdef  LOGTAG
	#define ESLOG(level, format, ...)	printf(M##level "[" LOGTAG "] %s(%d): " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
	#define ESLOG_HEX(level, _pHex_, _leng_)	printf(M##level "[" LOGTAG "] %s(%d): printHex\n", __FUNCTION__, __LINE__);\
												printfHex(_pHex_, _leng_)
	#define ESLOG_STRNONULL(level, format, _str_, _length_)	printf(M##level "[" LOGTAG "] %s(%d): " format , __FUNCTION__, __LINE__ );\
													    printfStrNoNULL(_str_, _length_)
#else
	#define ESLOG(level, format, ...)	if((level) < LOG_DEBUG) printf(M ##level " %s(%d): " format "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
	#define ESLOG_HEX(level, _pHex_, _leng_) 
	#define ESLOG_STRNONULL(level, format, _str_, _length_)	printf(M##level " %s(%d): " format , __FUNCTION__, __LINE__, );\
													    printfStrNoNULL(_str_, _length_)
#endif

void printfHex(void* pHexArray, int length);

void printfStrNoNULL(const char* str, const int length);

#endif
