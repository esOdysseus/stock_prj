#ifndef _SREQ_H_SUB_CLASS_EUNSEOK_KIM_
#define _SREQ_H_SUB_CLASS_EUNSEOK_KIM_

#include "Config.h"
#include <pthread.h>
#include <Windows.h>

typedef enum REQ_STATE
{
	REQ_ST_NULL = 0,		// 상태가 정해지지 않은 request
	REQ_ST_DEL = 1,			// loop 에서 이 request의 list node를 삭제하란 뜻.
	REQ_ST_READY = 2,		// request는 send할 준비가 되었음.
	REQ_ST_BUSY = 3,		// request는 사용중임.
	REQ_ST_SENT = 4,		// request는 send 되었음.
	REQ_ST_RECEIVING = 5,	// response를 나누어서 수신중임.
	REQ_ST_ERROR = 6		// request는 문제가 있음. (send 실패)
} REQ_STATE;

class sREQ
{
private:
	REQ_STATE    state;
	pthread_mutex_t mutex_state = PTHREAD_MUTEX_INITIALIZER;

public:
	SYSTEMTIME   sendTime;		// request를 보낼수 있는 최단 시간.
	int   reqID;		// request ID number (Send전 : 0, Send&처리중 : > 0 )
	char  sendType;		// request , requestService 중 어떤건지 구별한다. (1: request, 2:requestService)
	char* TRcode;		// Transaction Code.
	void* pReq_s;		// Request의 payload가 될 구조체 pointer.
	int Reqlength;		// pReq_s 의 payload 길이.(unit : charactor)
	int NextReq;		// 최초검색 : 0, 연속검색 : 1

	char* ExpRepDataType;	// request에 대해서, 기대하는 response들중, state를 REQ_ST_RECEVING으로 Set할수 있는 data-type name.
	long  RepRcdCnt;		// REQ_ST_RECEIVING일때, 수신할 Record에 대한 Count 정보를 나타낸다.
	void* RepUsercxt;		// REQ_ST_RECEIVING이 되었을때, User가 원하는 형태로 사용할 데이터 이다.

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