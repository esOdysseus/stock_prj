#include <stdio.h>

#include "dlogger.h"

static const char* cHEXFORMAT10 = "\t\t%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X : %s\n";
static const char* cHEXFORMAT9 = "\t\t%02X %02X %02X %02X %02X %02X %02X %02X %02X    : %s\n";
static const char* cHEXFORMAT8 = "\t\t%02X %02X %02X %02X %02X %02X %02X %02X       : %s\n";
static const char* cHEXFORMAT7 = "\t\t%02X %02X %02X %02X %02X %02X %02X          : %s\n";
static const char* cHEXFORMAT6 = "\t\t%02X %02X %02X %02X %02X %02X             : %s\n";
static const char* cHEXFORMAT5 = "\t\t%02X %02X %02X %02X %02X                : %s\n";
static const char* cHEXFORMAT4 = "\t\t%02X %02X %02X %02X                   : %s\n";
static const char* cHEXFORMAT3 = "\t\t%02X %02X %02X                      : %s\n";
static const char* cHEXFORMAT2 = "\t\t%02X %02X                         : %s\n";
static const char* cHEXFORMAT1 = "\t\t%02X                            : %s\n";

void printfHex(void* pHexArray, int length)
{
	int i = 0;
	char str[11] = { NULL, };
	unsigned char* pStr = (unsigned char*)pHexArray;

	if (pStr == NULL || length == 0)
	{
		printf("pStr(0x%X) or length(%d) is NULL.\n", pStr, length);
		return;
	}

	for (i = 0; i+10 <= length; i+=10)
	{
		int start = i;

		for (;start < i+10; start++)
		{
			char ch = '.';
			if ( 0x20 <= pStr[start] && pStr[start] < 0x7E)
				ch = pStr[start];

			str[start-i] = ch;
		}
		str[start-i] = NULL;

		printf(cHEXFORMAT10, pStr[i], pStr[i + 1], pStr[i + 2], pStr[i + 3], pStr[i + 4], pStr[i + 5], pStr[i + 6], pStr[i + 7], pStr[i + 8], pStr[i + 9], str);
	}

	int start = i;

	for (; start < length; start++)
	{
		char ch = '.';
		if (0x20 <= pStr[start] && pStr[start] < 0x7E)
			ch = pStr[start];

		str[start - i] = ch;
	}
	str[start-i] = NULL;

	switch (length - i)
	{
	case 9:
		printf(cHEXFORMAT9, pStr[i], pStr[i + 1], pStr[i + 2], pStr[i + 3], pStr[i + 4], pStr[i + 5], pStr[i + 6], pStr[i + 7], pStr[i + 8], str);
		break;
	case 8:
		printf(cHEXFORMAT8, pStr[i], pStr[i + 1], pStr[i + 2], pStr[i + 3], pStr[i + 4], pStr[i + 5], pStr[i + 6], pStr[i + 7], str);
		break;
	case 7:
		printf(cHEXFORMAT7, pStr[i], pStr[i + 1], pStr[i + 2], pStr[i + 3], pStr[i + 4], pStr[i + 5], pStr[i + 6], str);
		break;
	case 6:
		printf(cHEXFORMAT6, pStr[i], pStr[i + 1], pStr[i + 2], pStr[i + 3], pStr[i + 4], pStr[i + 5], str);
		break;
	case 5:
		printf(cHEXFORMAT5, pStr[i], pStr[i + 1], pStr[i + 2], pStr[i + 3], pStr[i + 4], str);
		break;
	case 4:
		printf(cHEXFORMAT4, pStr[i], pStr[i + 1], pStr[i + 2], pStr[i + 3], str);
		break;
	case 3:
		printf(cHEXFORMAT3, pStr[i], pStr[i + 1], pStr[i + 2], str);
		break;
	case 2:
		printf(cHEXFORMAT2, pStr[i], pStr[i + 1], str);
		break;
	case 1:
		printf(cHEXFORMAT1, pStr[i], str);
		break;
	}

}

void printfStrNoNULL(const char* str, const int length)
{
	if (length == 0 || str == NULL)
		return;

	for (int i = 0; i < length; i++)
	{
		printf("%c", str[i]);
	}

	printf("\n");
}
