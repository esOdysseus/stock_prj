#include <malloc.h>
#include <Windows.h>

#include "EScommon.h"

#define LOGTAG	"EScommon"
#include "dlogger.h"

void* ESmalloc(size_t size)
{
	void* res = NULL;

	if (size <= 0)
	{
		ESLOG(LOG_ERROR, "size(%d) is invalid.", size);
		return NULL;
	}

	res = malloc(size);
	memset(res, 0, size);
	return res;
}

void ESfree(void* pmem)
{
	if (pmem == NULL)
	{
		return;
	}

	free(pmem);
}

char* ESstrdup(const char* target)
{
	if (target == NULL)
	{
		return NULL;
	}

	return _strdup(target);
}

void* ESmemdup(const void* src, const size_t size)
{
	if (src == NULL || size <= 0)
	{
		return NULL;
	}

	void* dest = ESmalloc(size);
	if (dest == NULL)
	{
		ESLOG(LOG_ERROR, "memory allocation is failed.(size=%d)", size);
		return NULL;
	}

	memcpy(dest, src, size);
	return dest;
}

///////////////////////////////////////////////////////////////////////
//wchar_t ���� char ���� ����ȯ �Լ�
LPSTR ConvertWCtoC(LPCWSTR s)
{
	if (s == NULL) return NULL;
	
	int size = sizeof(wchar_t) * lstrlenW(s);
	char* pStr = NULL;
	
	if (size <= 0)
	{
		ESLOG(LOG_ERROR, "size is invalid.(%d)", size);
		return NULL;
	}
	
	//ESLOG(LOG_DEBUG, "origin = %s", s);
	//ESLOG(LOG_DEBUG, "size = %d", size);
	
	if ((pStr = (LPSTR)ESmalloc(size+1)) == NULL)
	{
		ESLOG(LOG_ERROR, "pStr is NULL");
		return NULL;
	}
	memcpy(pStr, (LPSTR)s, size);
	
	//ESLOG(LOG_DEBUG, "pStr = %s", pStr);
	
	return pStr;
}


//char * ConvertWCtoC(wchar_t* str)
//{
//	ESLOG(LOG_DEBUG, "Enter. str = %s, length=%d", str, lstrlenW(str););
//	int cw = lstrlenW(str);
//	//��ȯ�� char* ���� ����
//	char* pStr = NULL;
//
//	//�Է¹��� wchar_t ������ ���̸� ����
//	int strSize = WideCharToMultiByte(CP_ACP, 0, str, cw, NULL, 0, NULL, NULL);
//	ESLOG(LOG_DEBUG, "strSize = %d", strSize);
//
//	//char* �޸� �Ҵ�
//	pStr = (char*)ESmalloc(sizeof(char)*(2*strSize+1));
//	memset(pStr, 0, 2*strSize+1);
//
//	//�� ��ȯ 
//	WideCharToMultiByte(CP_ACP, 0, str, cw, pStr, strSize, 0, 0);
//	pStr[2*strSize] = NULL;
//	ESLOG(LOG_DEBUG, "pStr = %s", pStr);
//
//	ESLOG(LOG_DEBUG, "Exit.");
//	return pStr;
//}

///////////////////////////////////////////////////////////////////////
//char ���� wchar_t ���� ����ȯ �Լ�
wchar_t* ConverCtoWC(char* str)
{
	//ESLOG(LOG_DEBUG, "Enter.");

	//wchar_t�� ���� ����
	wchar_t* pStr;
	//��Ƽ ����Ʈ ũ�� ��� ���� ��ȯ
	int strSize = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, NULL);

	//wchar_t �޸� �Ҵ�
	//pStr = new WCHAR[strSize];
	pStr = (wchar_t*)ESmalloc(sizeof(wchar_t)*strSize);

	//�� ��ȯ
	MultiByteToWideChar(CP_ACP, 0, str, strlen(str) + 1, pStr, strSize);

	//ESLOG(LOG_DEBUG, "Exit.");
	return pStr;
}


char* GetCloneStringData(char* data, int length)
{
	char* clone = NULL;

	if (data == NULL || length == 0)
	{
		ESLOG(LOG_DEBUG, "Invalid input arguments.(length= %d, data=%s)", length, data);
		return clone;
	}

	while (length > 0 && data[length - 1] == ' ')	length--;

	if (length > 0)
	{
		if ((clone = (char*)ESmalloc(length + 1)) != NULL)
		{
			strncpy_s(clone, length + 1, data, length);
		}
		else
		{
			ESLOG(LOG_ERROR, "memory allocation is failed.");
		}
	}

	return clone;
}

long GetCloneLongData(char* data, int length)
{
	int i = 0;
	long res = -1;
	char* clone = NULL;

	if (data == NULL || length == 0)
	{
		ESLOG(LOG_DEBUG, "Invalid input arguments.(length= %d, data=%s)", length, data);
		return res;
	}

	while (i < (length-1) && (data[i] == '0' || data[i] == ' '))	i++;

	length = length - i;
	if (length > 0)
	{
		if ((clone = (char*)ESmalloc(length + 1)) != NULL)
		{
			strncpy_s(clone, length + 1, data+i, length);
			//ESLOG(LOG_DEBUG, "clone = %s", clone);
			res = atol(clone);
		}
		else
		{
			ESLOG(LOG_ERROR, "memory allocation is failed.");
		}
	}

	ESfree(clone);
	clone = NULL;

	return res;
}


void CloneValueString(char* dest, const char* src, int destBufSize)
{
	ESLOG(LOG_DEBUG, "Enter.-memcpy-");
	memcpy((void*)dest, (void*)src, destBufSize);
}

void CloneValueString(char* dest, const char src, int destBufSize)
{
	ESLOG(LOG_DEBUG, "Enter.-memset-");
	memset((void*)dest, src, destBufSize);
}

bool CloneValueString(char* dest, int src, int destBufSize)
{
	ESLOG(LOG_DEBUG, "Enter.-itoa-");

	char* buf = NULL;
	
	if ((buf = (char*)ESmalloc(sizeof(char)*(destBufSize + 1))) == NULL)
	{
		ESLOG(LOG_ERROR, "Memory allocatioin is failed.");
		return false;
	}

	_itoa_s(src, buf, destBufSize+1, 10);
	memcpy(dest, buf, destBufSize);
	ESfree(buf);

	return true;
}