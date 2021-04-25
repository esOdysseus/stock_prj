#ifndef _CSTOCKMAIN_H_BY_EUNSEOK
#define _CSTOCKMAIN_H_BY_EUNSEOK

#include <Windows.h>

#include "CSTReqResManager.h"
#include "CMessageHandler.h"

class CStockMain : public CMessageHandler
{
private:
	void*			Instance;		// xingAPI instance.
	void*			iotInstance;	// IoTivityAPI instance.
	HWND			pHwnd;			// 사용할 Window의 Handler.
	bool			bInit;			// 초기화
	bool            bConnect;		// 접속 여부
	bool            bLogin;			// 로그인 여부
	void*			threadHandle;	// SockMainThread handle
	bool			threadContinue;	// SockMainThread run flag
	CSTReqResManager* ReqResManager;

public:
	CStockMain(HWND hWnd);

	~CStockMain(void);

	bool isInit(void);

	bool ST_Login(void);

	bool ST_Logout(int timeOut);

	bool ST_Terminate(int LogOutTimeout);

	virtual bool IoT_ReqParse(REQ_CMDLIST method, void* data, TransactionDataType transNum);

protected:
	DECLARE_MESSAGE_MAP();

private:
	int StockMainThread(void);

	void ST_waitLogOut(int WaitSeconds);

	bool ST_GetAccountInfo(void);

	// Call-Back Function.
	LRESULT OnLoginCB(WPARAM wParam, LPARAM lParam);

	LRESULT OnLogoutCB(WPARAM wParam, LPARAM lParam);

	LRESULT OnSockDisconnCB(WPARAM wParam, LPARAM lParam);

	LRESULT OnReceiveDataCB(WPARAM wParam, LPARAM lParam);

	LRESULT OnReceiveRealDataCB(WPARAM wParam, LPARAM lParam);
};



#endif