#include "Config.h"
#include <pthread.h>

#define LOGTAG "CStockMain"
#include "dlogger.h"

#include "CStockMain.h"
#include "IXingAPI.h"
#include "IoTivityAPI.h"

#include "EScommon.h"
#include "CSTReqResManager.h"

//------------------------------------------------------------
// 접속 주소 및 포트
//#define SERVER_IP                       "hts.etrade.co.kr"
//#define SERVER_IP                       "demo.etrade.co.kr"	// 더이상 사용하지 않는 주소
#define SERVER_IP                       "demo.ebestsec.co.kr"
#define SERVER_PORT                     20001
#define STINSTANCE()					((IXingAPI*)Instance)
#define IOTINSTANCE()					((IoTivityAPI*)iotInstance)

#define XINGDLL_FLD						"XingDLL_x86"		// XingAPI DLL file folder name.
#define ACCOUNT_MAXEA					10

typedef int (CStockMain::*pCStockFuncType)(void);


static const char sUserID[] = "airmagic";						// 아이디
static const char sUserPwd[] = "6475kes-=";						// 비밀번호
static const char sCertPwd[] = "ssm06kes-=";					// 공인인증 비밀번호
static char* sUserACTs[ACCOUNT_MAXEA] = { NULL, };

BEGIN_MESSAGE_MAP(CStockMain)
	ON_MESSAGE(WM_USER + XM_LOGIN, OnLoginCB)
	ON_MESSAGE(WM_USER + XM_LOGOUT, OnLogoutCB)
	ON_MESSAGE(WM_USER + XM_DISCONNECT, OnSockDisconnCB)
	ON_MESSAGE(WM_USER + XM_RECEIVE_DATA, OnReceiveDataCB)
	ON_MESSAGE(WM_USER + XM_RECEIVE_REAL_DATA, OnReceiveRealDataCB)
END_MESSAGE_MAP()


//------------------------------------------------------------
// xingAPI를 초기화합니다
CStockMain::CStockMain(HWND hWnd)
{
	ESLOG(LOG_DEBUG, "Enter.");

	char CurDir[MAX_PATH] = { 0 };

	//reqParsorInit();		// init separator for request_parsor. (IoTivity GET/PUT/POST request)

	pHwnd = hWnd;
	Instance = NULL;
	bInit = false;			// 초기화
	bConnect = false;		// 접속 여부
	bLogin = false;			// 로그인 여부
	threadHandle = NULL;
	threadContinue = false;
	ReqResManager = NULL;

	if (pHwnd == NULL)
	{
		ESLOG(LOG_DEBUG, "pHwnd is NULL.");
		return;
	}

	// XingAPI initialization.
	if ((Instance = new IXingAPI()) == NULL)
	{
		ESLOG(LOG_DEBUG, "XingAPI Instance is NULL.");
		return;
	}

	GetCurrentDirectoryA(MAX_PATH, CurDir);
	strcat_s(CurDir, "\\" XINGDLL_FLD);
	ESLOG(LOG_DEBUG, "XingAPIPath = %s\n", CurDir);

	if (!STINSTANCE()->Init(pHwnd, CurDir))	// CurDir Directory에서 xingAPI.dll 을 load한다.
	{
		ESLOG(LOG_DEBUG, "XingAPI DLL을 로드할 수 없습니다.");
		delete STINSTANCE();
		Instance = NULL;
		return ;
	}

	if ((ReqResManager = new CSTReqResManager(Instance)) == NULL)
	{
		ESLOG(LOG_DEBUG, "ReqResManager is NULL. memory allocation failed.");
		STINSTANCE()->Uninit();
		delete STINSTANCE();
		Instance = NULL;
		return;
	}

	// IoTivity Initialization.
	if ((iotInstance = (void*)IoTivityAPI::GetInstance()) == NULL)
	{
		ESLOG(LOG_ERROR, "IoTivity library has a error.(Instance is NULL)");
		delete ReqResManager;
		ReqResManager = NULL;
		STINSTANCE()->Uninit();
		delete STINSTANCE();
		Instance = NULL;
		return;
	}

	if (IOTINSTANCE()->initialize(OC_CLIENT_SERVER, this) != 1)
	{
		ESLOG(LOG_ERROR, "IoTivity initialization is failed.");
		delete IOTINSTANCE();
		iotInstance = NULL;
		delete ReqResManager;
		ReqResManager = NULL;
		STINSTANCE()->Uninit();
		delete STINSTANCE();
		Instance = NULL;
		return;
	}

	bInit = true;

	ESLOG(LOG_DEBUG, "Exit.");
}

CStockMain::~CStockMain(void)
{
	ESLOG(LOG_DEBUG, "Enter.");

	ST_Terminate(0);
	ESLOG(LOG_DEBUG, "Exit.");
}

bool CStockMain::isInit(void)
{
	return bInit;
}

bool CStockMain::ST_Terminate(int LogOutTimeout)
{
	ESLOG(LOG_DEBUG, "Enter.");

	if (threadHandle != NULL)
	{
		int threadResult;
		threadContinue = false;
		pthread_join(*PTHREAD_HANDLE(threadHandle), (void**)&threadResult);
		ESLOG(LOG_DEBUG, "result of thread is \"%d\"", threadResult);
		ESfree(threadHandle);
		threadHandle = NULL;
	}

	while( ST_Logout(LogOutTimeout) == false )
		ESLOG(LOG_DEBUG, "retry Log-out");

	if (isInit() == true)
	{
		bInit = false;

		delete IOTINSTANCE();
		iotInstance = NULL;

		delete ReqResManager;
		ReqResManager = NULL;

		STINSTANCE()->Uninit();
		delete STINSTANCE();
		Instance = NULL;
	}

	ESLOG(LOG_DEBUG, "Exit.");
	return true;
}

//----------------------------------------------------------------------------------------------------
// 로그인 처리를 합니다
//----------------------------------------------------------------------------------------------------
bool CStockMain::ST_Login( void )
{
	ESLOG(LOG_DEBUG, "Enter.");

	if (isInit() == false)
	{
		ESLOG(LOG_DEBUG, "Not yet Initialized. check your system.");
		return false;
	}

	if (bLogin == true)
	{
		ESLOG(LOG_DEBUG, "Already Log-In completed to Server.");
		return true;
	}

	// 서버 접속을 합니다
	while(STINSTANCE()->IsConnected() == FALSE)
	{
		ESLOG(LOG_DEBUG, "run connect(%s)", SERVER_IP);

		bConnect = STINSTANCE()->Connect(SERVER_IP,	SERVER_PORT, WM_USER);

		if (!bConnect)
		{
			int nErrorCode = STINSTANCE()->GetLastError();
			char* strMsg = STINSTANCE()->GetErrorMessage(nErrorCode);

			ESLOG(LOG_ERROR, "Error of Stock-Server Connection.(%s)", strMsg);

			return false;
		}

		sleep(1);
	}

	// 로그인을 요청합니다
	// -> 로그인 성공여부는 로그인 메시지를 수신하여 확인합니다
	ESLOG(LOG_DEBUG, "run Login()");

	bool isSend = STINSTANCE()->Login(sUserID,
		sUserPwd,
		sCertPwd,
		0,
		FALSE);

	if (!isSend)
	{
		int nErrorCode = STINSTANCE()->GetLastError();
		char* strMsg = STINSTANCE()->GetErrorMessage(nErrorCode);

		ESLOG(LOG_ERROR, "Error of Stock-Server Login.(%s)", strMsg);
	}

	ESLOG(LOG_DEBUG, "Exit.");
	return isSend;
}


//----------------------------------------------------------------------------------------------------
// 로그아웃 처리를 합니다
//----------------------------------------------------------------------------------------------------
bool CStockMain::ST_Logout( int timeOut )
{
	ESLOG(LOG_DEBUG, "Enter.");

	if (isInit() == false)
	{
		ESLOG(LOG_INFO, "Not yet Initialized. check your system.");
		return true;
	}

	if (STINSTANCE()->IsConnected() == TRUE)
	{
		// 로그아웃을 요청합니다
		// -> 로그아웃 성공여부는 로그인 메시지를 수신하여 확인합니다
		if (bLogin == true)
		{
			ESLOG(LOG_DEBUG, "run Logout()");

			if (STINSTANCE()->Logout() == FALSE)
			{
				int nErrorCode = STINSTANCE()->GetLastError();
				char* strMsg = STINSTANCE()->GetErrorMessage(nErrorCode);

				ESLOG(LOG_ERROR, "Error of Stock-Server Logout.(%s)", strMsg);

				return FALSE;
			}

			ST_waitLogOut(timeOut);
		}

		ESLOG(LOG_DEBUG, "run Disconnect()");

		if (STINSTANCE()->Disconnect() == FALSE)
		{
			int nErrorCode = STINSTANCE()->GetLastError();
			char* strMsg = STINSTANCE()->GetErrorMessage(nErrorCode);

			ESLOG(LOG_ERROR, "Error of Stock-Server Disconnect.(%s)", strMsg);

			return FALSE;
		}
		bConnect = FALSE;
	}

	ESLOG(LOG_DEBUG, "Exit.");
	return TRUE;
}

//----------------------------------------------------------------------------------------------------
// [Call] IoTivity API 에서 호출한다. (원하는 request 및 인증을 위해서.)
//----------------------------------------------------------------------------------------------------
bool CStockMain::IoT_ReqParse(REQ_CMDLIST method, void* data, TransactionDataType transNum)
{
	ESLOG(LOG_DEBUG, "Enter.");

	;

	ESLOG(LOG_DEBUG, "Exit.");
	return true;
}

//----------------------------------------------------------------------------------------------------
// [Call-Back] 로그인 성공여부 메시지 처리를 합니다
//----------------------------------------------------------------------------------------------------
LRESULT CStockMain::OnLoginCB(WPARAM wParam, LPARAM lParam)
{
	ESLOG(LOG_DEBUG, "Enter.");

	//ESLOG(LOG_DEBUG, "wParam=%u  lParam=%u", wParam, lParam);

	LPCSTR pszCode = (LPCSTR)wParam;
	LPCSTR pszMsg = (LPCSTR)lParam;

	ESLOG(LOG_DEBUG, "wParam=%s", pszCode);
	ESLOG(LOG_DEBUG, "lParam=%s", pszMsg);

	if (atoi(pszCode) == 0)
	{
		// 로그인 성공
		ESLOG(LOG_INFO, "Log-in Success.");
		bLogin = true;

		// Stock-Task thread create
		pthread_attr_t threadAttr = NULL;
		if (pthread_attr_init(&threadAttr) == -1) {
			ESLOG(LOG_ERROR, "error in pthread_attr_init");
			exit(1);
		}

		if (pthread_attr_setstacksize(&threadAttr, 8192) == -1) {
			ESLOG(LOG_ERROR, "error in pthread_attr_setstacksize");
			pthread_attr_destroy(&threadAttr);
			exit(2);
		}
		
		if ( (threadHandle = ESmalloc(sizeof(pthread_t))) == NULL)
		{
			ESLOG(LOG_ERROR, "pthread_t malloc is failed.");
			pthread_attr_destroy(&threadAttr);
			exit(3);
		}

		//ESLOG(LOG_DEBUG, "This=0x%X", this);
		pCStockFuncType pCSockFunc = &CStockMain::StockMainThread;
		pThreadFuncType pFunc = *(pThreadFuncType*)&pCSockFunc;
		if (pthread_create(PTHREAD_HANDLE(threadHandle), &threadAttr, pFunc, (void*)this) == -1)
		{
			ESLOG(LOG_ERROR, "error in pthread_create for SockMainThread");
			pthread_attr_destroy(&threadAttr);
			ESfree(threadHandle);
			threadHandle = NULL;
			exit(4);
		}
		ESLOG(LOG_DEBUG, "this = 0x%X", this);

		pthread_attr_destroy(&threadAttr);
	}
	else
	{
		// 로그인 실패
		ESLOG(LOG_ERROR, "Log-in Failed.");
		bLogin = false;
	}

	ESLOG(LOG_DEBUG, "Exit.");
	return 1L;
}

//----------------------------------------------------------------------------------------------------
// [Call-Back] 로그아웃 성공여부 메시지 처리를 합니다
//----------------------------------------------------------------------------------------------------
LRESULT CStockMain::OnLogoutCB(WPARAM wParam, LPARAM lParam)
{
	ESLOG(LOG_DEBUG, "Stock Server Log-Out Success.");

	ESLOG(LOG_DEBUG, "wParam=%u  lParam=%u", wParam, lParam);

	bLogin = false;

	return 1;
}

//----------------------------------------------------------------------------------------------------
// [Call-Back] Win-Socket Disconnect 여부 메시지 처리를 합니다
//----------------------------------------------------------------------------------------------------
LRESULT CStockMain::OnSockDisconnCB(WPARAM wParam, LPARAM lParam)
{
	ESLOG(LOG_DEBUG, ">>>> [Occurred Socket Disconnected.] <<<<");

	bLogin = false;
	if (isInit() == true)
	{
		ESLOG(LOG_DEBUG, "Re-LogIN try.");

		while( ST_Login() == false )
			ESLOG(LOG_DEBUG, "Re-LogIN is failed.");
	}

	return 1;
}

//----------------------------------------------------------------------------------------------------
// [Call-Back] XM_RECEIVE_DATA 수신했을때, 메시지 처리를 합니다
//----------------------------------------------------------------------------------------------------
LRESULT CStockMain::OnReceiveDataCB(WPARAM wParam, LPARAM lParam)
{
	LRESULT result = true;
	RECV_PACKET* recvPacket = NULL;
	MSG_PACKET * msgPacket = NULL;
	STATE_RECV_DATA state = STATE_NO_DATA;

	ESLOG(LOG_DEBUG, "wParam=%u  lParam=%u", wParam, lParam);

	state = (STATE_RECV_DATA)wParam;
	switch (state)
	{
	case STATE_REQUEST_DATA:
		ESLOG(LOG_DEBUG, "Received RECV_PACKET data.(0x%X)", lParam);
		if ((recvPacket = (RECV_PACKET*)lParam) == NULL)
		{
			ESLOG(LOG_ERROR, "XingAPI Error: recvPacket is NULL.");
			result = false;
		}

		ESLOG(LOG_DEBUG, "reqID: 0x%X, TRcode: %s, Continue: %s, ContinueKey(NextKey): %s"
			           , recvPacket->nRqID, recvPacket->szTrCode, recvPacket->cCont, recvPacket->szContKey);
		ESLOG(LOG_DEBUG, "nDataMode = %d, BlockName=%s, recvDataSize=%d, memAllocedDataSize=%d", recvPacket->nDataMode, recvPacket->szBlockName, recvPacket->nDataLength, recvPacket->nTotalDataBufferSize);

		if (ReqResManager->CallDataHandler(recvPacket->nRqID, recvPacket->szBlockName, recvPacket->szTrCode, recvPacket->lpData, (void*)recvPacket->nDataLength) != 1)
		{
			ESLOG(LOG_ERROR, "Tere is not exist handler for %s", recvPacket->szTrCode);
			result = false;
		}
		break;
	case STATE_SYSTEM_ERROR_DATA:
		ESLOG(LOG_ERROR, "Occured error : STATE_SYSTEM_ERROR_DATA");
	case STATE_MESSAGE_DATA:
		ESLOG(LOG_DEBUG, "Occured State : %d", state);
		ESLOG(LOG_DEBUG, "Received MSG_PACKET data.(0x%X)", lParam);
		if( (msgPacket = (MSG_PACKET*)lParam) == NULL )
		{
			ESLOG(LOG_ERROR, "XingAPI Error: msgPacket is NULL.");
			result = false;
			break;
		}

		ESLOG(LOG_DEBUG, "reqID: 0x%X, type: %d, Code: %s", msgPacket->nRqID, msgPacket->nIsSystemError, msgPacket->szMsgCode);
		ESLOG(LOG_DEBUG, "Data = %s", msgPacket->lpszMessageData);

		if (state == STATE_SYSTEM_ERROR_DATA)
		{
			ReqResManager->RelPendingRequest(msgPacket->nRqID);
		}

		//if (msgPacket->nIsSystemError == 1 || msgPacket->szMsgCode != 0)	// TODO Search 언제가 정상 MsgCode인지를 알아야 한다.
		//{
		//	ESLOG(LOG_INFO, "Fail Xing-request(%d).", msgPacket->nRqID);

		//	ESLOG(LOG_INFO, " So, If needed, remove registered Observe list.");
		//	ReqResManager;	// TODO If needed, then Remove Observer List.
		//}

		if (STINSTANCE()->ReleaseMessageData(lParam) == false)
		{
			ESLOG(LOG_ERROR, "XingAPI Error: ReleaseMessageData is failed.");
			result = false;
		}
		break;
	//case STATE_SYSTEM_ERROR_DATA:
		//ESLOG(LOG_ERROR, "Occured error : STATE_SYSTEM_ERROR_DATA");
		//if ((msgPacket = (MSG_PACKET*)lParam) == NULL)
		//{
		//	ESLOG(LOG_ERROR, "XingAPI Error: msgPacket is NULL.");
		//	result = false;
		//	break;
		//}

		//ESLOG(LOG_DEBUG, "reqID: 0x%X, type: %d, Code: %s", msgPacket->nRqID, msgPacket->nIsSystemError, msgPacket->szMsgCode);
		//ESLOG(LOG_DEBUG, "Data = %s", msgPacket->lpszMessageData);

		//if (STINSTANCE()->ReleaseMessageData(lParam) == false)
		//{
		//	ESLOG(LOG_ERROR, "XingAPI Error: ReleaseMessageData is failed.");
		//	result = false;
		//}
		//break;
	case STATE_RELEASE_DATA:
		ESLOG(LOG_DEBUG, "Request ID : 0x%X", (int)lParam);
		if (STINSTANCE()->ReleaseRequestData((int)lParam) == false)
		{
			ESLOG(LOG_ERROR, "XingAPI Error: ReleaseMessageData is failed.");
			result = false;
		}
		break;
	default :
		ESLOG(LOG_ERROR, "Not Support State=%d", state);
		result = false;
		break;
	}

	return result;
}

//----------------------------------------------------------------------------------------------------
// [Call-Back] XM_RECEIVE_REAL_DATA 수신했을때, 메시지 처리를 합니다
//----------------------------------------------------------------------------------------------------
LRESULT CStockMain::OnReceiveRealDataCB(WPARAM wParam, LPARAM lParam)
{
	LRESULT result = true;
	RECV_REAL_PACKET* recvPacket = NULL;

	ESLOG(LOG_DEBUG, "wParam=%u  lParam=%u", wParam, lParam);

	if ((recvPacket = (LPRECV_REAL_PACKET)lParam) == NULL)
	{
		ESLOG(LOG_ERROR, "Received Real-Data from Stock-Server is NULL.");
		return false;
	}

	if (ReqResManager->CallRealDataHandler(recvPacket->szTrCode, recvPacket->pszData, (void*)recvPacket->nDataLength) != 1)
	{
		ESLOG(LOG_ERROR, "Tere is not exist handler for %s", recvPacket->szTrCode);
		result = false;
	}

	return result;
}





/**************************************************
*
*	Main Thread of Stock Class.
*
**************************************************/
int CStockMain::StockMainThread(void)
{
	// Getting Account Number.
	if (ST_GetAccountInfo() == false)
	{
		ESLOG(LOG_ERROR, "Getting Account Number is failed.");
		return 1;
	}

	threadContinue = true;

	int reqID = ReqResManager->SendTimeSearch();
	ESLOG(LOG_DEBUG, "request ID = 0x%X", reqID);
	//int reqID2 = ReqResManager->SendStockSearch(STE_SS_KOSPI);
	//int reqID3 = ReqResManager->SendOneStockSearch("005930");

	if (ReqResManager->AllowTradeOrder(sUserACTs[0], sUserPwd) == true)
	{
		ESLOG(LOG_DEBUG, "============= Test Trade Order ===================");
		//ReqResManager->SendTradeOrder(STE_TRADE_BUY, "005930", 1, 1790000);
	}
	
	if (reqID < 0)
	{
		ESLOG(LOG_ERROR, "Error XingAPI: Server-Time-Search is failed.");
	}

	//char* commMedia = STINSTANCE()->GetCommMedia();			// 통신매체를 구한다.
	//ESLOG(LOG_DEBUG, "commMedia = %s", commMedia);

	//char* ETKMedia   = STINSTANCE()->GetETKMedia();			// 당사매체를 구한다.
	//ESLOG(LOG_DEBUG, "ETKMedia = %s", ETKMedia);

	//char* clientIP   = STINSTANCE()->GetClientIP();			// 공인IP를 구한다.
	//ESLOG(LOG_DEBUG, "clientIP = %s", clientIP);

	//char* ServerName = STINSTANCE()->GetServerName();		// 서버명을 구한다.
	//ESLOG(LOG_DEBUG, "ServerName = %s", ServerName);

	//char* APIPath    = STINSTANCE()->GetAPIPath();			// 실행중인 xingAPI의 경로
	//ESLOG(LOG_DEBUG, "APIPath = %s", APIPath);

	int reqID4 = ReqResManager->SendChartSearch("005930", STE_PRID_1DAY,
		2017, 1, 13, 2017, 1, 16);
	ESLOG(LOG_DEBUG, "reqID : %d for SendChartSearch", reqID4);

	int reqID5 = ReqResManager->SendActBalanceSearch();
	ESLOG(LOG_DEBUG, "reqID : %d for SendActBalanceSearch", reqID5);

	while (threadContinue)
	{
		printf("@");
		if (ReqResManager->SendPendingRequest() == false)
		{
			ESLOG(LOG_ERROR, "Pending Request Sending is failed.");
			break;
		}
		sleep(1);
	}

	ESLOG(LOG_DEBUG, "****** Destroy Main Thread. ******");
	for (int i = 0; i < ACCOUNT_MAXEA; i++)
	{
		ESfree(sUserACTs[i]);
		sUserACTs[i] = NULL;
	}

	ESLOG(LOG_DEBUG, "Exit.");
	return 1;
}



void CStockMain::ST_waitLogOut(int WaitSeconds)
{
	if (bLogin == false)
		return;

	if (WaitSeconds == 0)
	{
		ESLOG(LOG_DEBUG, "Log-Out Success not wait.");
		return;
	}

	while (bLogin == true && WaitSeconds > 0)
	{
		ESLOG(LOG_DEBUG, "Waiting Log-out.(cnt=%d)", WaitSeconds);
		WaitSeconds--;
		sleep(1);
	}

	if (bLogin == true)
	{
		ESLOG(LOG_DEBUG, "Log-out waiting is Time-out.");
		bLogin = false;
	}
}


#define AC_DATA_SIZE		256
bool CStockMain::ST_GetAccountInfo(void)
{
	int i = 0;
	bool res = false;
	int acCNT = 0;

	if (bLogin == false)
		return res;

	acCNT = min( STINSTANCE()->GetAccountListCount(), ACCOUNT_MAXEA );		// 사용가능한 계정수를 알아오다.
	ESLOG(LOG_DEBUG, "account cnt = %d", acCNT);

	for (i = 0; i < acCNT; i++)
	{
		//char acData[AC_DATA_SIZE] = { 0, };
		char acNum[AC_DATA_SIZE] = { 0, };

		memset(acNum, 0, AC_DATA_SIZE);
		if (STINSTANCE()->GetAccountList(i, acNum, AC_DATA_SIZE-1) == false)
		{
			ESLOG(LOG_DEBUG, "GetAccountList is failed.");
			return false;
		}
		sUserACTs[i] = ESstrdup(acNum);
		ESLOG(LOG_DEBUG, "AC-%d Num: %s", i, sUserACTs[i]);

		//memset(acData, 0, AC_DATA_SIZE);
		//if (STINSTANCE()->GetAccountName(ConverCtoWC(sUserACTs[i]), acData, AC_DATA_SIZE - 1) == false)
		//{
		//	ESLOG(LOG_DEBUG, "GetAccountName is failed.");
		//	return false;
		//}
		//ESLOG(LOG_DEBUG, "AC-%d Name: %s", i, acData);

		//memset(acData, 0, AC_DATA_SIZE);
		//if (STINSTANCE()->GetAcctDetailName(ConverCtoWC(sUserACTs[i]), acData, AC_DATA_SIZE - 1) == false)
		//{
		//	ESLOG(LOG_DEBUG, "GetAcctDetailName is failed.");
		//	return false;
		//}
		//ESLOG(LOG_DEBUG, "AC-%d DetailName: %s", i, acData);

		res = true;
	}

	return res;
}



