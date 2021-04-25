#ifndef _SREQ_H_SUB_CLASS_EUNSEOK_KIM_
#define _SREQ_H_SUB_CLASS_EUNSEOK_KIM_

#include "Config.h"
#include <pthread.h>
#include <Windows.h>

typedef enum REQ_STATE
{
	REQ_ST_NULL = 0,		// ���°� �������� ���� request
	REQ_ST_DEL = 1,			// loop ���� �� request�� list node�� �����϶� ��.
	REQ_ST_READY = 2,		// request�� send�� �غ� �Ǿ���.
	REQ_ST_BUSY = 3,		// request�� �������.
	REQ_ST_SENT = 4,		// request�� send �Ǿ���.
	REQ_ST_RECEIVING = 5,	// response�� ����� ��������.
	REQ_ST_ERROR = 6		// request�� ������ ����. (send ����)
} REQ_STATE;

class sREQ
{
private:
	REQ_STATE    state;
	pthread_mutex_t mutex_state = PTHREAD_MUTEX_INITIALIZER;

public:
	SYSTEMTIME   sendTime;		// request�� ������ �ִ� �ִ� �ð�.
	int   reqID;		// request ID number (Send�� : 0, Send&ó���� : > 0 )
	char  sendType;		// request , requestService �� ����� �����Ѵ�. (1: request, 2:requestService)
	char* TRcode;		// Transaction Code.
	void* pReq_s;		// Request�� payload�� �� ����ü pointer.
	int Reqlength;		// pReq_s �� payload ����.(unit : charactor)
	int NextReq;		// ���ʰ˻� : 0, ���Ӱ˻� : 1

	char* ExpRepDataType;	// request�� ���ؼ�, ����ϴ� response����, state�� REQ_ST_RECEVING���� Set�Ҽ� �ִ� data-type name.
	long  RepRcdCnt;		// REQ_ST_RECEIVING�϶�, ������ Record�� ���� Count ������ ��Ÿ����.
	void* RepUsercxt;		// REQ_ST_RECEIVING�� �Ǿ�����, User�� ���ϴ� ���·� ����� ������ �̴�.

	sREQ(void);
	~sREQ(void);

	void SetRequestStruct(const int req_id, const REQ_STATE State, const char* trcode,
		const void* pReq, const int reqlength, const char SendType, const char* expRepDataTypeName);

	bool tryCASreqState(const REQ_STATE preState, const bool preRes, const REQ_STATE postState);

	void CASreqState(const REQ_STATE preState, const bool preRes, const REQ_STATE postState);

	REQ_STATE GETreqState(void);

	void SETreqState(const REQ_STATE postState);

	sREQ* CreateNextRequest(const void* req, const int reqLength, const int delaySec);
};



#endif // _SREQ_H_SUB_CLASS_EUNSEOK_KIM_