
#define LOGTAG "CMessageHandler"
#include "dlogger.h"

#include "CMessageHandler.h"


bool CMessageHandler::ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	bool ret = false;
	int i = 0;
	const MSG_MSGMAP_ENTRY* pmsgMap = GetMessageMap();

	if ( pmsgMap == NULL )
	{
		ESLOG(LOG_INFO, "pmsgMap Not initialized.");
		return ret;
	}

	//ESLOG(LOG_DEBUG, "Enter.");

	while (pmsgMap[i].nMessage != 0 && pmsgMap[i].pfn != NULL && pmsgMap[i].nSig != MsgSig_end)
	{
		if (pmsgMap[i].nMessage == message)
		{
			ESLOG(LOG_INFO, "Found Function point int Message Map.(messge = %d)", message);
			ret = RunMessageFunc(pmsgMap[i].nSig, pmsgMap[i].pfn, wParam, lParam);

			break;
		}

		i++;
	}

	//ESLOG(LOG_DEBUG, "Exit.");

	return ret;
}


bool CMessageHandler::RunMessageFunc(UINT_PTR funcType, PMSGFunc pFunc, WPARAM wParam, LPARAM lParam)
{
	bool ret = false;

	if (pFunc == NULL)
	{
		ESLOG(LOG_ERROR, "pFunc is NULL.");
		return ret;
	}

	switch (funcType)
	{
	case MsgSig_l_w_l:			//	typedef LRESULT(theClass::*ThisClassCB)(WPARAM wParam, LPARAM lParam);
		{
			ESLOG(LOG_DEBUG, "pFunc Type :  LRESULT(CMessageHandler::*ThisClassCB)(WPARAM wParam, LPARAM lParam)");
			typedef LRESULT(CMessageHandler::*ThisClassCB)(WPARAM wParam, LPARAM lParam);
			ThisClassCB pfunc = reinterpret_cast<ThisClassCB>(pFunc);
			(this->*pfunc)(wParam, lParam);
		}
		ret = true;
		break;
	default :
		ESLOG(LOG_ERROR, "Not Support funcType.(%u)", funcType);
		break;
	}

	return ret;
}
