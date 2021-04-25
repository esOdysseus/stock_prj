
#include "sREQ.h"

#define LOGTAG	"sREQ"
#include "dlogger.h"

#include "EScommon.h"


sREQ::sREQ( void )
{
	pthread_mutex_init(&mutex_state, NULL);
	SETreqState(REQ_ST_NULL);
	TRcode = NULL;
	pReq_s = NULL;
	Reqlength = 0;
	NextReq = 0;
	sendType = 0;
	reqID = 0;
	ExpRepDataType = NULL;
	RepRcdCnt = 0;
	RepUsercxt = 0;
}

sREQ::~sREQ( void )
{
	SETreqState(REQ_ST_NULL);
	ESfree(TRcode);
	TRcode = NULL;
	ESfree(pReq_s);
	pReq_s = NULL;
	Reqlength = 0;
	NextReq = 0;
	sendType = 0;
	reqID = 0;
	ExpRepDataType = NULL;
	RepRcdCnt = 0;
	RepUsercxt = 0;
	pthread_mutex_destroy(&mutex_state);
}

void sREQ::SetRequestStruct(const int req_id, const REQ_STATE State, const char* trcode,
	const void* pReq, const int reqlength, const char SendType, const char* expRepDataTypeName)
{
	if (State != REQ_ST_NULL)
		SETreqState(State);
	
	if (pReq != NULL)
		pReq_s = (void*)pReq;
	if (reqlength != 0)
		Reqlength = reqlength;
	if (SendType != 0)
		sendType = SendType;

	if (trcode != NULL)
	{
		ESfree(TRcode);
		TRcode = ESstrdup(trcode);
	}

	if (expRepDataTypeName != NULL)
	{
		ESfree(ExpRepDataType);
		ExpRepDataType = ESstrdup(expRepDataTypeName);
	}

	reqID = req_id;
}

bool sREQ::tryCASreqState(const REQ_STATE preState, const bool preRes, const REQ_STATE postState)
{
	bool res = false;

	pthread_mutex_lock(&mutex_state);
	if ((state == preState) == preRes)
	{
		state = postState;
		res = true;
	}
	pthread_mutex_unlock(&mutex_state);

	return res;
}

void sREQ::CASreqState(const REQ_STATE preState, const bool preRes, const REQ_STATE postState)
{
	while (!tryCASreqState(preState, preRes, postState))
	{
		sleep(1);
		//usleep(300000);		// 0.3 sec delay
	}
}

REQ_STATE sREQ::GETreqState(void)
{
	REQ_STATE State = REQ_ST_NULL;

	pthread_mutex_lock(&mutex_state);
	State = state;
	pthread_mutex_unlock(&mutex_state);

	return State;
}

void sREQ::SETreqState(const REQ_STATE postState)
{
	pthread_mutex_lock(&mutex_state);
	state = postState;
	pthread_mutex_unlock(&mutex_state);
}

sREQ* sREQ::CreateNextRequest(const void* req, const int reqLength, const int delaySec)
{
	ESLOG(LOG_DEBUG, "Enter.");

	sREQ* next = NULL;

	if (req == NULL || reqLength == 0 || delaySec < 0 || delaySec > 60)
	{
		ESLOG(LOG_ERROR, "Invalid Input argument.(req=0x%X, reqLength=%d, delaySec=%d)", req, reqLength, delaySec);
		return next;
	}

	void* CloneReq = ESmalloc(sizeof(char) * reqLength);
	if (CloneReq == NULL)
	{
		ESLOG(LOG_ERROR, "Memory allocation is failed.(CloneReq)");
		return next;
	}
	memcpy(CloneReq, req, reqLength);

	if ((next = new sREQ()) == NULL)
	{
		ESLOG(LOG_ERROR, "Memory allocation is failed.");
		ESfree(CloneReq);
		CloneReq = NULL;
		return next;
	}

	GetSystemTime(&next->sendTime);
	if (delaySec)
	{
		next->sendTime.wSecond += delaySec;
		if (next->sendTime.wSecond >= 60)
		{
			next->sendTime.wSecond = next->sendTime.wSecond - 60;
			next->sendTime.wMinute += 1;

			if (next->sendTime.wMinute >= 60)
			{
				next->sendTime.wMinute = 0;
				next->sendTime.wHour += 1;
				if (next->sendTime.wHour >= 24)
				{
					next->sendTime.wHour = 0;
					next->sendTime.wDay += 1;
				}
			}
		}
	}

	next->SetRequestStruct(0, REQ_ST_READY, TRcode, CloneReq, reqLength, sendType, ExpRepDataType);
	next->NextReq = 1;			// 연속 검색

	ESLOG(LOG_DEBUG, "Exit.");
	return next;
}