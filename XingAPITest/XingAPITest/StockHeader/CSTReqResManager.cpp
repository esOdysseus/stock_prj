#include <string.h>

#include "IXingAPI.h"
#include "CSTReqResManager.h"

#define LOGTAG	"CSTReqResManager"
#include "dlogger.h"

#include "EScommon.h"

static inline void registerTOrderDefOBVs(IXingAPI* xingAPI);
static inline void unregisterTOrderDefOBVs(IXingAPI* xingAPI);
static int Def_RegiUnregiTOrderStockOBV(IXingAPI* xingAPI, bool isRegister, const char* stockCode);
//static char* GetChartIndexName(STE_INDEXNAME graph);
static sREQ* CreateChartRequestData(const char* stockcode, STE_PERIOD period,
			int sYear, int sMon, int sDay, int eYear, int eMon, int eDay,
			int sHour, int sMin, int sSec, int eHour, int eMin, int eSec);
static int compareSysTime(WORD src, WORD des);
static int compareSystemTime(SYSTEMTIME& src, SYSTEMTIME& des);
static void hadleCRecordTickSearchData(STChartTickSearchOut2* data, int dataSize, sREQ* req);
static void hadleCRecordMinSearchData(STChartMinSearchOut2* data, int dataSize, sREQ* req);
static void hadleCRecordDaysSearchData(STChartDaysSearchOut2* data, int dataSize, sREQ* req);


CSTReqResManager::CSTReqResManager(void)
{
	xingAPIins = NULL;
	accountNum = NULL;
	passNum = NULL;
	TradeState = STE_OBVST_DEINITED;
	Mutex_StockOBVMap = PTHREAD_MUTEX_INITIALIZER;
	Mutex_TradeState = PTHREAD_MUTEX_INITIALIZER;
}

CSTReqResManager::CSTReqResManager(void* xingIns)
{
	xingAPIins = xingIns;
	accountNum = NULL;
	passNum = NULL;
	TradeState = STE_OBVST_DEINITED;
	Mutex_StockOBVMap = PTHREAD_MUTEX_INITIALIZER;
	Mutex_TradeState = PTHREAD_MUTEX_INITIALIZER;

	// register Received-Data Event handler function.
	handlerDataMap[TRCODE_STOCK_SEARCH]		= (handlerFuncType)&CSTReqResManager::handleStockSearch;
	handlerDataMap[TRCODE_TIME_SEARCH]		= (handlerFuncType)&CSTReqResManager::handleTimeSearch;
	handlerDataMap[TRCODE_ONESTOCK_SEARCH]	= (handlerFuncType)&CSTReqResManager::handleOneStockSearch;
	handlerDataMap[TRCODE_TRADE_ORDER]		= (handlerFuncType)&CSTReqResManager::handleTradeOrder;
	handlerDataMap[TRCODE_CHART_TICKSEARCH]     = (handlerFuncType)&CSTReqResManager::handleChartSearch;
	handlerDataMap[TRCODE_CHART_MINSEARCH] = (handlerFuncType)&CSTReqResManager::handleChartSearch;
	handlerDataMap[TRCODE_CHART_DAYSSEARCH] = (handlerFuncType)&CSTReqResManager::handleChartSearch;
	handlerDataMap[TRCODE_ACTBALANCE_SEARCH] = (handlerFuncType)&CSTReqResManager::handleActBalanceSearch;

	// register Received-Real-Data Event handler function.
	handlerRealDataMap[TRCODE_ORDER_ACT]	  = (handlerRealFuncType)&CSTReqResManager::handleDummy;
	handlerRealDataMap[TRCODE_ORDER_SUCCESS]  = (handlerRealFuncType)&CSTReqResManager::handleTOrdSuccess;
	handlerRealDataMap[TRCODE_ORDER_CHANGED]  = (handlerRealFuncType)&CSTReqResManager::handleTOrdChanged;
	handlerRealDataMap[TRCODE_ORDER_CANCEL]	  = (handlerRealFuncType)&CSTReqResManager::handleTOrdCancel;
	handlerRealDataMap[TRCODE_ORDER_REJECTED] = (handlerRealFuncType)&CSTReqResManager::handleTOrdRejected;

	handlerRealDataMap[TRCODE_KOSPI_SUCCESS]  = (handlerRealFuncType)&CSTReqResManager::handleKPISuccess;
	handlerRealDataMap[TRCODE_KOSDAQ_SUCCESS] = (handlerRealFuncType)&CSTReqResManager::handleKDAQSuccess;
	handlerRealDataMap[TRCODE_KOSPI_REMAIN]   = (handlerRealFuncType)&CSTReqResManager::handleKPIRemain;
	handlerRealDataMap[TRCODE_KOSDAQ_REMAIN]  = (handlerRealFuncType)&CSTReqResManager::handleKDAQRemain;

	//handlerRealDataMap[TRCODE_CHART_SEARCH]   = (handlerFuncType)&CSTReqResManager::handleChartSearch;
}

CSTReqResManager::~CSTReqResManager(void)
{
	xingAPIins = NULL;
	ESfree(accountNum);
	ESfree(passNum);
	TradeState = STE_OBVST_DEINITED;
	Mutex_StockOBVMap = PTHREAD_MUTEX_INITIALIZER;
	Mutex_TradeState = PTHREAD_MUTEX_INITIALIZER;
}

bool CSTReqResManager::AllowTradeOrder(const char* account, const char* passwd)
{
	if (account == NULL || passwd == NULL)
	{
		ESLOG(LOG_ERROR, "Invalid arguments.");
		goto GOTO_OUT;
	}

	if (TradeState != STE_OBVST_DEINITED)
	{
		ESLOG(LOG_INFO, "Already inited for Trade-Order.");
		goto GOTO_OUT;
	}

	ESfree(accountNum);
	ESfree(passNum);
	accountNum = NULL;
	passNum = NULL;

	if ((accountNum = ESstrdup(account)) == NULL)
	{
		ESLOG(LOG_ERROR, "memory allocation is failed. accountNum.");
		goto GOTO_OUT;
	}
	
	if( (passNum = ESstrdup(passwd)) == NULL )
	{
		ESLOG(LOG_ERROR, "memory allocation is failed. passNum.");
		goto GOTO_OUT;
	}

	TradeState = STE_OBVST_INITED;

GOTO_OUT:
	return TradeState>=STE_OBVST_INITED? true: false;
}

/**
* @brief  Call handler-function base on TR-code for Received-Data from Stock-server.
* @param trCode   TR-code. (Transaction Code number)
* @param data     Received result-data pointer from Stock-Server.
* @return         return value of handler-function
*/
int CSTReqResManager::CallDataHandler(int reqID, char* dataType, char* trCode, void* data1, void* data2)
{
	ESLOG(LOG_DEBUG, "Enter.");

	int result = 0;
	handlerFuncType  func = NULL;
	sREQ* request = NULL;
	SentReqMapType::iterator itor;

	if (trCode == NULL || data1 == NULL)
	{
		ESLOG(LOG_ERROR, "Invalid input arguments.(trCode=%s, data=0x%X)", trCode, data1);
		return 0;
	}

	if (reqID > 0)
	{
		ESLOG(LOG_DEBUG, "Received request ID number.(%d)", reqID);
		itor = SentRequestMap.find(reqID);
		if ( itor != SentRequestMap.end())
		{
			if ((request = SentRequestMap[reqID]) == NULL)
			{
				ESLOG(LOG_ERROR, "Invalid : request is NULL.");
				return 0;
			}
		}
		else
		{
			ESLOG(LOG_DEBUG, "Not Exist request-structure.");
		}
	}

	if ((func = handlerDataMap[trCode]) != NULL)
	{
		result = (this->*func)(request, dataType, data1, data2);
	}
	else
	{
		ESLOG(LOG_ERROR, "Not support TRcode(%s).", trCode);
	}

	if (request != NULL && request->GETreqState() == REQ_ST_DEL && itor != SentRequestMap.end())
	{
		ESLOG(LOG_DEBUG, "Erase request in SentRequestMap.");
		SentRequestMap.erase(itor);
	}

	ESLOG(LOG_DEBUG, "Exit.");

	return result;
}

/**
* @brief  Call handler-function base on TR-code for Received-Real-Data from Stock-server.
* @param trCode   TR-code. (Transaction Code number)
* @param data     Received result-data pointer from Stock-Server.
* @return         return value of handler-function
*/
int CSTReqResManager::CallRealDataHandler(char* trCode, void* data1, void* data2)
{
	ESLOG(LOG_DEBUG, "Enter.");

	int result = 0;
	handlerRealFuncType  func = NULL;

	if (trCode == NULL || data1 == NULL)
	{
		ESLOG(LOG_DEBUG, "Invalid input arguments.(trCode=%s, data=0x%X)", trCode, data1);
		return 0;
	}

	if ((func = handlerRealDataMap[trCode]) != NULL)
	{
		result = (this->*func)(data1, data2);
	}
	else
	{
		ESLOG(LOG_ERROR, "Not support TRcode(%s).", trCode);
	}

	ESLOG(LOG_DEBUG, "Exit.");

	return result;
}

bool CSTReqResManager::SendPendingRequest(void)
{
	//ESLOG(LOG_DEBUG, "Enter.");

	SYSTEMTIME cTime;
	PenReqMapType::iterator itor = PendingRequestMap.begin();

	GetSystemTime(&cTime);

	while ( itor != PendingRequestMap.end() )
	{
		sREQ* req = (sREQ*)*itor;

		if (req == NULL)
		{
			ESLOG(LOG_ERROR, "List-Contents is NULL.");
			return false;
		}

		if (req->GETreqState() == REQ_ST_NULL)
		{
			ESLOG(LOG_ERROR, "Invalid State value.(It's REQ_ST_NULL)");
			return false;
		}

		if (req->tryCASreqState(REQ_ST_READY, true, REQ_ST_BUSY) == true)
		{	// If state Ready, then sending process start.
			ESLOG(LOG_DEBUG, "Sending request Start.");

			if (req->TRcode == NULL || req->pReq_s == NULL || req->Reqlength == 0)
			{
				ESLOG(LOG_ERROR, "Invalid request contents.(TRcode=0x%X, req=0x%X, length=%d)", req->TRcode, req->pReq_s, req->Reqlength);
				return false;
			}

			if (compareSystemTime(cTime, req->sendTime) != -1)
			{	// not meet sending time for request.
				ESLOG(LOG_DEBUG, "Not yet meet time of send.");
				req->SETreqState(REQ_ST_READY);
				itor++;
				continue;
			}

			switch (req->sendType)
			{
			case 1: // request
				req->reqID = ((IXingAPI*)xingAPIins)->Request(req->TRcode, req->pReq_s, req->Reqlength, req->NextReq);
				break;
			case 2: // requestService
				req->reqID = ((IXingAPI*)xingAPIins)->RequestService(req->TRcode, (char*)req->pReq_s);
				break;
			default:
				ESLOG(LOG_ERROR, "Not Supported Sending Type method.(%d)", req->sendType);
			}

			if (req->reqID > 0)
			{
				ESLOG(LOG_DEBUG, "Sending request is Success.(reqID=%d)", req->reqID);
				SentRequestMap[req->reqID] = req;
				req->SETreqState(REQ_ST_SENT);
			}
			else
			{
				ESLOG(LOG_ERROR, "Error: sending request is failed.");
				req->reqID = 0;
				req->SETreqState(REQ_ST_ERROR);
			}
		}
		else if (req->tryCASreqState(REQ_ST_DEL, true, REQ_ST_BUSY) == true)
		{	// If state Delete, then Erasing process start.
			ESLOG(LOG_DEBUG, "Deleting request Start.");
			PendingRequestMap.erase(itor++);
			delete req;
			continue;
		}

		itor++;
	}

	//ESLOG(LOG_DEBUG, "Exit.");
	return true;
}

void CSTReqResManager::RelPendingRequest(int reqID)
{
	ESLOG(LOG_DEBUG, "Enter.");

	sREQ* request = NULL;
	SentReqMapType::iterator itor;

	if (reqID > 0)
	{
		ESLOG(LOG_DEBUG, "Received request ID number.(%d)", reqID);
		itor = SentRequestMap.find(reqID);
		if (itor != SentRequestMap.end())
		{
			if ((request = SentRequestMap[reqID]) != NULL)
			{
				ESLOG(LOG_ERROR, "set request-state to REQ_ST_DEL.");
				request->SETreqState(REQ_ST_DEL);
			}

			ESLOG(LOG_DEBUG, "Erase request in SentRequestMap.");
			SentRequestMap.erase(itor);
		}
		else
		{
			ESLOG(LOG_DEBUG, "Not Exist request-structure.");
		}
	}

	ESLOG(LOG_DEBUG, "Exit.");
}


/*********************************************************************************************************
*
*	[시작] Received Data Event를 위해서 송/수신된 데이터를 처리하는 request/handler들을 나열한 것이다.
*
*********************************************************************************************************/
/*****************************************************
*
*	"주식 종목 조회"를 위한 Sender/Handler Definition.
*
*****************************************************/
/**
* @brief  Call-Back function to send request to Stock-Server for searching Stock contents.
* @param target   0 : all \n
                  1 : Cospi \n
				  2 : Costak
* @return         Success : request-ID value
                  Failure : -1
*/
int CSTReqResManager::SendStockSearch(STE_STOCKSEARCH market)
{
	ESLOG(LOG_DEBUG, "Enter.");

	int reqID = 0;
	int length = sizeof(STStockSearchIn);
	STStockSearchIn reqData;

	if (xingAPIins == NULL || market < STE_SS_ALL || market > STE_SS_KOSDAQ )
	{
		ESLOG(LOG_DEBUG, "Invalid input arguments.(xingIns=0x%X, target=%d)", xingAPIins, market);
		return -1;
	}

	memset(&reqData, 0, length);
	reqData.gubun[0] = market;
	reqID = ((IXingAPI*)xingAPIins)->Request(TRCODE_STOCK_SEARCH,&reqData, sizeof(STStockSearchIn));
	if (reqID < 0)
	{
		ESLOG(LOG_DEBUG, "request is failed.");
		return -1;
	}

	ESLOG(LOG_DEBUG, "Exit.");

	return reqID;
}

/**
 * @brief  Call-Back function to handle result-data received from Stock-Server. (for Stock conponents search)
 * @param data  Result Data received from stock-Server.
 * @return  Success : 1
            Failure : 0
			Error   : -1
 */
int CSTReqResManager::handleStockSearch(sREQ* request, char* dataType, STStockSearchOut* data, int dataSize)
{
	ESLOG(LOG_DEBUG, "Enter.");
	
	int res = -1;
	int dataEA = 0;
	char* hname = NULL;
	char* gubun = NULL;
	char* shcode = NULL;
	char* expcode = NULL;
	char* recprice = NULL;
	char* uplmtprice = NULL;
	STStockSearchOut* pData = data;
	
	if (pData == NULL || dataSize == 0)
	{
		ESLOG(LOG_DEBUG, "Data is NULL.");
		return 0;
	}

	dataEA = dataSize / sizeof(STStockSearchOut);
	ESLOG(LOG_DEBUG, "received buffer Size= %d, EA of data = %d", dataSize, dataEA);

	while (dataEA--)
	{
		//ESLOG_HEX(LOG_DEBUG, pData, sizeof(STStockSearchOut));

		hname = GetCloneStringData(pData->hname, sizeof(pData->hname));
		gubun = GetCloneStringData(pData->gubun, sizeof(pData->gubun));
		shcode = GetCloneStringData(pData->shcode, sizeof(pData->shcode));
		expcode = GetCloneStringData(pData->expcode, sizeof(pData->expcode));
		recprice = GetCloneStringData(pData->recprice, sizeof(pData->recprice));
		uplmtprice = GetCloneStringData(pData->uplmtprice, sizeof(pData->uplmtprice));

		if (hname == NULL || gubun == NULL || shcode == NULL || expcode == NULL || recprice == NULL || uplmtprice == NULL)
		{
			ESLOG(LOG_DEBUG, "memory allocation is failed.(hname=0x%X, gubun=0x%X)", hname, gubun);
			ESLOG(LOG_DEBUG, "(shcode=0x%X, expcode=0x%X, recprice=0x%X, uplmtprice=0x%X)", shcode, expcode, recprice, uplmtprice);
			goto GOTO_OUT;
		}

		if (strstr(hname, "삼성전자") != NULL)
		{
			ESLOG(LOG_DEBUG, "hname=%s", hname);
			//ESLOG(LOG_DEBUG, "gubun=%s", gubun);
			ESLOG(LOG_DEBUG, "shcode=%s", shcode);
			//ESLOG(LOG_DEBUG, "expcode=%s", expcode);
			//ESLOG(LOG_DEBUG, "recprice=%s", recprice);
			//ESLOG(LOG_DEBUG, "uplmtprice=%s", uplmtprice);
		}

		ESfree(hname);
		ESfree(gubun);
		ESfree(shcode);
		ESfree(expcode);
		ESfree(recprice);
		ESfree(uplmtprice);

		hname = NULL;
		gubun = NULL;
		shcode = NULL;
		expcode = NULL;
		recprice = NULL;
		uplmtprice = NULL;

		pData++;
	}

	res = 1;

GOTO_OUT:
	ESfree(hname);
	ESfree(gubun);
	ESfree(shcode);
	ESfree(expcode);
	ESfree(recprice);
	ESfree(uplmtprice);

	ESLOG(LOG_DEBUG, "Exit.");
	return res;
}


/*****************************************************
*
*	"Server 시간 조회"를 위한 Sender/Handler Definition.
*
*****************************************************/
/**
* @brief  Call-Back function to send request to Stock-Server for searching Stock contents.
* @return         Success : request-ID value
                  Failure : -1
*/
int CSTReqResManager::SendTimeSearch(void)
{
	ESLOG(LOG_DEBUG, "Enter.");

	int reqID = 0;
	int length = sizeof(STTimeSearchIn);
	STTimeSearchIn reqData;
	
	if (xingAPIins == NULL)
	{
		ESLOG(LOG_DEBUG, "XingAPI instance is NULL.");
		return -1;
	}

	memset(&reqData, 0, length);
	reqID = ((IXingAPI*)xingAPIins)->Request(TRCODE_TIME_SEARCH, &reqData, length);
	if (reqID < 0)
	{
		ESLOG(LOG_DEBUG, "request is failed.");
		return -1;
	}

	ESLOG(LOG_DEBUG, "Exit.");

	return reqID;
}

/**
 * @brief  Call-Back function to handle result-data received from Stock-Server. (for Time search)
 * @param data  Result Data received from stock-Server.
 * @return  Success : 1
            Failure : 0
			Error   : -1
 */
static bool setCurTime(const char* date, const char* time)
{
	int Year = 0;
	int Month = 0;
	int Days = 0;
	int Hour = 0;
	int Min = 0;
	int Sec = 0;
	int curDate = 0;
	int curTime = 0;
	SYSTEMTIME sTime;

	if (atoi(date) <= 0 || atoi(time) <= 0)
	{
		ESLOG(LOG_DEBUG, "date or time is not number.");
		return false;
	}

	curDate = atoi(date);
	curTime = atoi(time);
	ESLOG(LOG_DEBUG, "curDate=%d, curTime=%d", curDate, curTime);

	// Setting Date
	Year = curDate / 10000;
	Month = (curDate - (Year * 10000)) / 100;
	Days = curDate - (Year * 10000) - (Month * 100);

	ESLOG(LOG_DEBUG, "Year=%d, Month=%d, Days=%d", Year, Month, Days);

	sTime.wYear = Year;
	sTime.wMonth = Month;
	sTime.wDay = Days;

	// Setting Time
	Hour = curTime / 10000;
	Min = (curTime - (Hour * 10000)) / 100;
	Sec = curTime - (Hour * 10000) - (Min * 100);

	ESLOG(LOG_DEBUG, "Hour=%d, Min=%d, Sec=%d", Hour, Min, Sec);

	sTime.wHour = Hour;
	sTime.wMinute = Min;
	sTime.wSecond = Sec;

	return (SetSystemTime(&sTime) == 0) ? false : true;
}

int CSTReqResManager::handleTimeSearch(sREQ* request, char* dataType, STTimeSearchOut* data, int dataSize)
{
	ESLOG(LOG_DEBUG, "Enter.");

	int res = -1;
	int dataEA = 0;
	char* date = NULL;
	char* time = NULL;
	STTimeSearchOut* pData = data;

	if (pData == NULL || dataSize == 0)
	{
		ESLOG(LOG_DEBUG, "Data is NULL.");
		return 0;
	}

	dataEA = dataSize / sizeof(STTimeSearchOut);
	ESLOG(LOG_DEBUG, "received buffer Size= %d, EA of data = %d", dataSize, dataEA);

	while (dataEA--)
	{
		date = GetCloneStringData(pData->dt, sizeof(pData->dt));
		time = GetCloneStringData(pData->time, sizeof(pData->time));

		if (date == NULL || time == NULL)
		{
			ESLOG(LOG_DEBUG, "memory allocation is failed.(date=0x%X, time=0x%X)", date, time);
			goto GOTO_OUT;
		}

		ESLOG(LOG_DEBUG, "date=%s", date);
		ESLOG(LOG_DEBUG, "time=%s", time);

		if (setCurTime(date, time) == false)
		{
			ESLOG(LOG_DEBUG, "Setting System time is failed.");
			goto GOTO_OUT;
		}

		ESfree(date);
		ESfree(time);

		date = NULL;
		time = NULL;

		pData++;
	}

	res = 1;

GOTO_OUT:
	ESfree(date);
	ESfree(time);

	ESLOG(LOG_DEBUG, "Exit.");
	return res;
}




/***********************************************
*
*	"특정 주식의 현재가 조회"를 위한 Definition.
*
***********************************************/
/**
* @brief  Call function to send request to Stock-Server for searching about specific One-Stock.
* @return         Success : request-ID value
			      Failure : -1
*/
int CSTReqResManager::SendOneStockSearch(const char* stockCode)
{
	ESLOG(LOG_DEBUG, "Enter.");

	int reqID = 0;
	int length = sizeof(STOneSearchIn);
	STOneSearchIn reqData;

	if (xingAPIins == NULL || stockCode == NULL)
	{
		ESLOG(LOG_DEBUG, "XingAPI instance is NULL.(0x%X) or Invalid argument.(0x%X)", xingAPIins, stockCode);
		return -1;
	}

	if (strlen(stockCode) > sizeof(reqData.shcode) || strlen(stockCode) == 0)
	{
		ESLOG(LOG_DEBUG, "Invalid arguement.(length=%d)(It must 0 < x <= %d)", strlen(stockCode), sizeof(reqData.shcode));
		return -1;
	}

	ESLOG(LOG_DEBUG, "Stock Code = %s", stockCode);

	memset(&reqData, 0, length);
	memcpy(reqData.shcode, stockCode, strlen(stockCode));
	reqID = ((IXingAPI*)xingAPIins)->Request(TRCODE_ONESTOCK_SEARCH, &reqData, length);
	if (reqID < 0)
	{
		ESLOG(LOG_DEBUG, "request is failed.");
		return -1;
	}

	ESLOG(LOG_DEBUG, "Exit.");

	return reqID;
}

/**
* @brief  Call-Back function to handle result-data received from Stock-Server. (for specific One-Stock search)
* @param data      Result Data received from stock-Server.
* @param dataSize  Received data-size from Stock-Server.
* @return  Success : 1
		   Failure : 0
		   Error   : -1
*/
int CSTReqResManager::handleOneStockSearch(sREQ* request, char* dataType, STOneSearchOut* data, int dataSize)
{
	ESLOG(LOG_DEBUG, "Enter.");

	int res = -1;
	int dataEA = 0;
	char* hname = NULL;
	char* price = NULL;
	STOneSearchOut* pData = data;

	if (pData == NULL || dataSize == 0)
	{
		ESLOG(LOG_DEBUG, "Data is NULL.");
		return 0;
	}

	dataEA = dataSize / sizeof(STOneSearchOut);
	ESLOG(LOG_DEBUG, "received buffer Size= %d, EA of data = %d", dataSize, dataEA);

	while (dataEA--)
	{
		hname = GetCloneStringData(pData->hname, sizeof(pData->hname));
		price = GetCloneStringData(pData->price, sizeof(pData->price));

		if (hname == NULL || price == NULL)
		{
			ESLOG(LOG_DEBUG, "memory allocation is failed.(hname=0x%X, price=0x%X)", hname, price);
			goto GOTO_OUT;
		}

		ESLOG(LOG_DEBUG, "hname=%s", hname);
		ESLOG(LOG_DEBUG, "price=%s", price);

		ESfree(hname);
		ESfree(price);

		hname = NULL;
		price = NULL;

		pData++;
	}

	res = 1;

GOTO_OUT:
	ESfree(hname);
	ESfree(price);

	ESLOG(LOG_DEBUG, "Exit.");
	return res;
}


/***********************************************
*
*	"현물 정상주문"를 위한 Definition.
*
***********************************************/
/**
* @brief  Call function to send request to Stock-Server for searching about specific One-Stock.
* @return         Success : request-ID value
				  Failure : -1
*/
int CSTReqResManager::SendTradeOrder(STE_TRADE mode, const char* stockCode, const unsigned int stockEA, const unsigned int pricePerStock)
{
	ESLOG(LOG_DEBUG, "Enter.");

	int reqID = -1;
	int length = sizeof(STTradeOrderIn);
	STTradeOrderIn reqData;
	int lengthStockCd = 0;

	if (xingAPIins == NULL || stockCode == NULL)
	{
		ESLOG(LOG_ERROR, "XingAPI instance is NULL.(0x%X) or Invalid argument.(0x%X)", xingAPIins, stockCode);
		return -1;
	}

	if (TradeState < STE_OBVST_INITED)
	{
		ESLOG(LOG_ERROR, "Not allowed sending trade order to Sock-Server. please use AllowTradeOrder() Function.");
		return -1;
	}

	lengthStockCd = strlen(stockCode);
	if (lengthStockCd  == 0 || lengthStockCd > sizeof(reqData.IsuNo) )
	{
		ESLOG(LOG_ERROR, "Invalid arguement.(length=%d)(It must 0 < x <= %d)", lengthStockCd, sizeof(reqData.IsuNo));
		return -1;
	}

	ESLOG(LOG_DEBUG, "Stock Code = %s", stockCode);

	memset(&reqData, ' ', length);
	switch (mode)
	{
	case STE_TRADE_BUY:
		reqData.BnsTpCode[0] = '1';
		break;
	case STE_TRADE_SELL:
		reqData.BnsTpCode[0] = '2';
		break;
	default :
		ESLOG(LOG_ERROR, "Not Support STE_TRADE type(%d)", mode);
		return -1;
	}
	memcpy(reqData.AcntNo, accountNum, strlen(accountNum));
	memcpy(reqData.InptPwd, passNum, strlen(passNum));
	memcpy(reqData.IsuNo, stockCode, lengthStockCd);
	if (_ltoa_s(stockEA, reqData.OrdQty, sizeof(reqData.OrdQty), 10) != 0)
	{
		ESLOG(LOG_ERROR, "failed ltoa(stockEA: %d, %s)", stockEA, reqData.OrdQty);
		return -1;
	}
	if (_ltoa_s(pricePerStock, reqData.OrdPrc, sizeof(reqData.OrdPrc), 10) != 0)
	{
		ESLOG(LOG_ERROR, "failed ltoa(pricePerStock: %d, %s)", pricePerStock, reqData.OrdPrc);
		return -1;
	}
	memcpy(reqData.OrdprcPtnCode, "00", 2);
	memcpy(reqData.MgntrnCode, "000", 3);
	memcpy(reqData.LoanDt, "        ", 8);
	memcpy(reqData.OrdCndiTpCode, "0", 1);

	if (RegisterObservers(stockCode) == true)
	{
		reqID = ((IXingAPI*)xingAPIins)->Request(TRCODE_TRADE_ORDER, &reqData, length);
		if (reqID < 0)
		{
			ESLOG(LOG_DEBUG, "request is failed.");
			return -1;
		}
	}

	ESLOG(LOG_DEBUG, "Exit.");

	return reqID;
}


/**
* @brief  Call-Back function to handle result-data received from Stock-Server. (for specific One-Stock search)
* @param data      Result Data received from stock-Server.
* @param dataSize  Received data-size from Stock-Server.
* @return  Success : 1
Failure : 0
Error   : -1
*/
int CSTReqResManager::handleTradeOrder(sREQ* request, char* dataType, void* data, int dataSize)
{
	ESLOG(LOG_DEBUG, "Enter.");

	int res = 0;

	//ESLOG(LOG_DEBUG, "data size = %d", dataSize);
	//ESLOG_HEX(LOG_DEBUG, data, dataSize);

	int dataEA = 0;
	char* stockCode = NULL;
	char* orderNum = NULL;
	STTradeOrderOut1* pData1 = (STTradeOrderOut1*)data;
	STTradeOrderOut2* pData2 = NULL;

	if (pData1 == NULL || dataSize == 0)
	{
		ESLOG(LOG_DEBUG, "Data is NULL.");
		return 0;
	}

	dataEA = dataSize / (sizeof(STTradeOrderOut1)+ sizeof(STTradeOrderOut2));
	ESLOG(LOG_DEBUG, "STTradeOrderOut1 Size= %d, STTradeOrderOut2 Size = %d", sizeof(STTradeOrderOut1), sizeof(STTradeOrderOut2));
	ESLOG(LOG_DEBUG, "received buffer Size= %d, EA of data = %d", dataSize, dataEA);

	if (dataEA != 1)
	{
		ESLOG(LOG_ERROR, "received data size is invalid.");
		return -1;
	}

	pData2 = (STTradeOrderOut2*)((char*)data + sizeof(STTradeOrderOut1));

	ESLOG(LOG_DEBUG, "STTradeOrderOut1 Hex print.");
	ESLOG_HEX(LOG_DEBUG, pData1, sizeof(STTradeOrderOut1));
	ESLOG(LOG_DEBUG, "STTradeOrderOut2 Hex print.");
	ESLOG_HEX(LOG_DEBUG, pData2, sizeof(STTradeOrderOut2));

	// 주문 접수 실패 여부를 알기 위해서 Stock-Server에서 발급된 orderNumber를 획득해서 처리한다.
	if ((stockCode = GetCloneStringData(pData1->IsuNo, sizeof(pData1->IsuNo))) == NULL)
	{
		ESLOG(LOG_ERROR, "GetCloneStringData() is failed.");
		return -1;
	}
	
	orderNum = GetCloneStringData(pData2->OrdNo, sizeof(pData2->OrdNo));
	ESLOG(LOG_DEBUG, "stockCode=%s, orderNum=%s ", stockCode, orderNum);
	if (orderNum == NULL)
	{	// Order Number가 없을시 주문 접수 실패로 간주한다.
		ESLOG(LOG_INFO, "Stock-Server not accept Trade-Order. So, unregister stock-Observor.");
		unRegisterObservers(STE_OBVST_NONE, stockCode);
	}
	else
	{
		ESLOG(LOG_INFO, "Stock-Server accept Trade-Order. Server will process Trade-Order.");
		//hname = GetCloneStringData(pData->hname, sizeof(pData->hname));
		//price = GetCloneStringData(pData->price, sizeof(pData->price));

		//if (hname == NULL || price == NULL)
		//{
		//	ESLOG(LOG_DEBUG, "memory allocation is failed.(hname=0x%X, price=0x%X)", hname, price);
		//	goto GOTO_OUT;
		//}

		//ESLOG(LOG_DEBUG, "hname=%s", hname);
		//ESLOG(LOG_DEBUG, "price=%s", price);

		//ESfree(hname);
		//ESfree(price);
		//hname = NULL;
		//price = NULL;

		//pData++;
	}

	res = 1;

	ESfree(stockCode);
	ESfree(orderNum);

	ESLOG(LOG_DEBUG, "Exit.");
	return res;
}

/***********************************************
*
*	"Index Chart형식 데이터 조회"를 위한 Definition.
*
***********************************************/
/**
* @brief  Call function to send request to Stock-Server for searching about specific One-Stock.
* @return         Success : request-ID value
Failure : -1
*/
int CSTReqResManager::SendChartSearch( const char* stockCode, STE_PERIOD period,
	                                  int sYear, int sMon, int sDay, int eYear, int eMon, int eDay,
									  int sHour, int sMin, int sSec, int eHour, int eMin, int eSec )
{
	ESLOG(LOG_DEBUG, "Enter.");

	int reqID = 0;
	sREQ* req = NULL;

	if (xingAPIins == NULL || stockCode == NULL || 
		sYear < 1900 || eYear < 1900 || 
		sMon < 1 || sMon > 12 || eMon < 1 || eMon > 12 || 
		sDay < 1 || sDay > 31 || eDay < 1 || eDay > 31 )
	{
		ESLOG(LOG_ERROR, "XingAPI instance is NULL.(0x%X) or Invalid argument.(stockCode=%s, sYear=%d, eYear=%d, sMon=%d, eMon=%d, sDay=%d, eDay=%d)", 
			                                                       xingAPIins, stockCode, sYear, eYear, sMon, eMon, sDay, eDay);
		return -1;
	}

	if ( strlen(stockCode) == 0 )
	{
		ESLOG(LOG_ERROR, "Invalid arguement.(length=%d)", strlen(stockCode));
		return -1;
	}

	if (sYear < 1900 || sYear > eYear || (sYear == eYear && sMon > eMon) || (sYear == eYear && sMon == eMon && sDay > eDay))
	{
		ESLOG(LOG_ERROR, "Wrong Date.(Start Date: %d-%d-%d , End Date: %d-%d-%d)", sYear, sMon, sDay, eYear, eMon, eDay);
		return -1;
	}

	ESLOG(LOG_DEBUG, "Stock Code = %s", stockCode);

	if ( (req = CreateChartRequestData(stockCode, period, sYear, sMon, sDay, eYear, eMon, eDay, sHour, sMin, sSec, eHour, eMin, eSec)) == NULL)
	{
		ESLOG(LOG_ERROR, "CreateChartRequestData() is failed.");
		return -1;
	}

	ESLOG_HEX(LOG_DEBUG, (void*)req->pReq_s, req->Reqlength);

	PendingRequestMap.insert(PendingRequestMap.begin(), req);

	//reqID = ((IXingAPI*)xingAPIins)->Request(req->TRcode, (char*)req->pReq_s, req->Reqlength);
	//if (reqID < 0)
	//{
	//	ESLOG(LOG_ERROR, "request is failed.");
	//	return -1;
	//}

	//req->reqID = reqID;

	ESLOG(LOG_DEBUG, "Exit.");
	return reqID;
}

/**
* @brief  Call-Back function to handle result-data received from Stock-Server. (for specific One-Stock search)
* @param data      Result Data received from stock-Server.
* @param dataSize  Received data-size from Stock-Server.
* @return  Success : 1
Failure : 0
Error   : -1
*/
int CSTReqResManager::handleChartSearch(sREQ* request, char* dataType, void* data, int dataSize)
{
	ESLOG(LOG_DEBUG, "Enter.");

	REQ_STATE state = REQ_ST_NULL;

	ESLOG(LOG_DEBUG, "data size = %d, DataType TickSize = %d, MinSize = %d, DaysSize = %d", dataSize, sizeof(STChartTickSearchOut1), sizeof(STChartMinSearchOut1), sizeof(STChartDaysSearchOut1));
	//ESLOG_HEX(LOG_DEBUG, data, dataSize);

	if (request == NULL || dataType == NULL)
	{
		ESLOG(LOG_ERROR, "Error: Invalid argument.(request=0x%X, dataType=0x%X)", request, dataType);
		return 0;
	}

	if (request->ExpRepDataType == NULL )
	{
		ESLOG(LOG_ERROR, "Error: Invalid argument.(ExpRepDataType=0x%X)", request->ExpRepDataType);
		return 0;
	}

	ESLOG(LOG_DEBUG, "request ID          : %d", request->reqID );
	ESLOG(LOG_DEBUG, "request State       : %d", request->GETreqState());
	ESLOG(LOG_DEBUG, "reponse OutTypeName : %s", dataType);

	state = request->GETreqState();
	switch (state)
	{
	case REQ_ST_SENT:
		ESLOG(LOG_DEBUG, "Need to change State to REQ_ST_RECEIVING.");
		ESLOG(LOG_DEBUG, "request OutTypeName : %s", request->ExpRepDataType);

		ESLOG_HEX(LOG_DEBUG, data, dataSize);

		if (strcmp(dataType, request->ExpRepDataType) == 0)
		{
			sREQ* NextReq = NULL;
			char* ChangeDate = NULL;

			if (strcmp(dataType, STChartTickSearchOut1Name) == 0)
			{
				STChartTickSearchIn*  Input = (STChartTickSearchIn*)request->pReq_s;
				STChartTickSearchOut1* Out  = (STChartTickSearchOut1*)data;
				request->RepRcdCnt = GetCloneLongData(Out->rec_count, sizeof(Out->rec_count));
				memcpy(Input->cts_date, Out->cts_date, sizeof(Out->cts_date));
				memcpy(Input->cts_time, Out->cts_time, sizeof(Out->cts_time));
				ChangeDate = GetCloneStringData(Input->cts_date, sizeof(Input->cts_date));
			}
			else if (strcmp(dataType, STChartMinSearchOut1Name) == 0)
			{
				STChartMinSearchIn*  Input = (STChartMinSearchIn*)request->pReq_s;
				STChartMinSearchOut1* Out = (STChartMinSearchOut1*)data;
				request->RepRcdCnt = GetCloneLongData(Out->rec_count, sizeof(Out->rec_count));
				memcpy(Input->cts_date, Out->cts_date, sizeof(Out->cts_date));
				memcpy(Input->cts_time, Out->cts_time, sizeof(Out->cts_time));
				ChangeDate = GetCloneStringData(Input->cts_date, sizeof(Input->cts_date));
			}
			else if (strcmp(dataType, STChartDaysSearchOut1Name) == 0)
			{
				STChartDaysSearchIn*  Input = (STChartDaysSearchIn*)request->pReq_s;
				STChartDaysSearchOut1* Out = (STChartDaysSearchOut1*)data;
				request->RepRcdCnt = GetCloneLongData(Out->rec_count, sizeof(Out->rec_count));
				memcpy(Input->cts_date, Out->cts_date, sizeof(Out->cts_date));
				ChangeDate = GetCloneStringData(Input->cts_date, sizeof(Input->cts_date));
			}
			request->SETreqState(REQ_ST_RECEIVING);

			ESLOG(LOG_DEBUG, "records=%d, ChangeDate=%s", request->RepRcdCnt, ChangeDate);
			if (ChangeDate != NULL)
			{
				ESLOG(LOG_DEBUG, "Start next request creating.");
				if ((NextReq = request->CreateNextRequest(request->pReq_s, request->Reqlength, 1)) != NULL)
				{
					ESLOG(LOG_DEBUG, "Insert Next Request to Pending-ReqeustList.");
					ESLOG_HEX(LOG_DEBUG, (void*)NextReq->pReq_s, NextReq->Reqlength);

					PendingRequestMap.insert(PendingRequestMap.begin(), NextReq);
				}

				ESfree(ChangeDate);
				ChangeDate = NULL;
			}
		}
		break;
	case REQ_ST_RECEIVING:
		ESLOG(LOG_DEBUG, "It's State is REQ_ST_RECEIVING.");

		if (strcmp(dataType, STChartTickSearchOut2Name) == 0)
		{
			hadleCRecordTickSearchData((STChartTickSearchOut2*)data, dataSize, request);
		}
		else if (strcmp(dataType, STChartMinSearchOut2Name) == 0)
		{
			hadleCRecordMinSearchData((STChartMinSearchOut2*)data, dataSize, request);
		}
		else if (strcmp(dataType, STChartDaysSearchOut2Name) == 0)
		{
			hadleCRecordDaysSearchData((STChartDaysSearchOut2*)data, dataSize, request);
		}

		request->SETreqState(REQ_ST_DEL);
		break;
	default :
		ESLOG(LOG_ERROR, "Not Supported request-State.(%d)", state);
		return 0;
	}

	ESLOG(LOG_DEBUG, "Exit.");
	return 1;
}

/***********************************************
*
*	"계좌 잔고 조회"를 위한 Definition.
*
***********************************************/
int CSTReqResManager::SendActBalanceSearch(void)
{
	ESLOG(LOG_DEBUG, "Enter.");

	sREQ* req = NULL;
	int reqID = -1;
	int length = sizeof(STActBalSearchIn);
	STActBalSearchIn* reqData;
	int lengthStockCd = 0;

	if (xingAPIins == NULL )
	{
		ESLOG(LOG_ERROR, "XingAPI instance is NULL.(0x%X).", xingAPIins);
		return -1;
	}

	if ((req = new sREQ()) == NULL)
	{
		ESLOG(LOG_ERROR, "sREQ request memory allocation is failed.");
		return -1;
	}

	if ( (reqData = (STActBalSearchIn*)ESmalloc(length)) == NULL)
	{
		ESLOG(LOG_ERROR, "STActBalSearchIn memory allocation is failed.");
		return -1;
	}

	memset(reqData, ' ', length);
	CloneValueString(reqData->AcntNo, accountNum, sizeof(reqData->AcntNo));
	CloneValueString(reqData->Pwd, passNum, sizeof(reqData->Pwd));
	CloneValueString(reqData->BalCreTp,		'1', sizeof(reqData->BalCreTp));
	CloneValueString(reqData->CmsnAppTpCode, '1', sizeof(reqData->CmsnAppTpCode));
	CloneValueString(reqData->D2balBaseQryTp,'1', sizeof(reqData->D2balBaseQryTp));
	CloneValueString(reqData->UprcTpCode,	'0', sizeof(reqData->UprcTpCode));

	req->SetRequestStruct(0, REQ_ST_READY, TRCODE_ACTBALANCE_SEARCH, reqData, length, 1, NULL);

	ESLOG_HEX(LOG_DEBUG, (void*)req->pReq_s, req->Reqlength);

	PendingRequestMap.insert(PendingRequestMap.begin(), req);

	//reqID = ((IXingAPI*)xingAPIins)->Request(TRCODE_ACTBALANCE_SEARCH, &reqData, length);
	//if (reqID < 0)
	//{
	//	ESLOG(LOG_DEBUG, "request is failed.");
	//	return -1;
	//}

	ESLOG(LOG_DEBUG, "Exit.");

	return reqID;
}

int CSTReqResManager::handleActBalanceSearch(sREQ* request, char* dataType, void* data, int dataSize)
{
	ESLOG(LOG_DEBUG, "Enter.");

	STActBalSearchOut1* OutData1;
	STActBalSearchOut2* OutData2;

	if (request == NULL)
	{
		ESLOG(LOG_ERROR, "Error: Invalid argument.(request=0x%X)", request);
		return 0;
	}

	ESLOG(LOG_DEBUG, "data size = %d, dataType=%s", dataSize, dataType);
	ESLOG_HEX(LOG_DEBUG, data, dataSize);

	OutData1 = (STActBalSearchOut1*)data;
	OutData2 = (STActBalSearchOut2*)((char*)data+sizeof(STActBalSearchOut1));

	char* PlaceName = GetCloneStringData(OutData2->BrnNm, sizeof(OutData2->BrnNm));
	char* ActName   = GetCloneStringData(OutData2->AcntNm, sizeof(OutData2->AcntNm));
	long CurOutableMoney = GetCloneLongData(OutData2->MnyoutAbleAmt, sizeof(OutData2->MnyoutAbleAmt));
	long D2Mymoney = GetCloneLongData(OutData2->D2Dps, sizeof(OutData2->D2Dps));

	ESLOG(LOG_DEBUG, "==================================");
	ESLOG(LOG_DEBUG, "Place=%s, ActName=%s, OutableMoney=%d, Mymoney=%d", PlaceName, ActName, CurOutableMoney, D2Mymoney);
	ESLOG(LOG_DEBUG, "==================================");

	request->SETreqState(REQ_ST_DEL);

	ESLOG(LOG_DEBUG, "Exit.");
	return 1;
}
/*********************************************************************************************************
*
*	[끝] Received Data Event를 위해서 송/수신된 데이터를 처리하는 request/handler들을 나열한 것이다.
*
*********************************************************************************************************/




bool CSTReqResManager::RegisterObservers(const char* stockCode)
{
	bool res = true;

	pthread_mutex_lock(&Mutex_TradeState);
	switch (TradeState)
	{
	case STE_OBVST_DEINITED:
		ESLOG(LOG_DEBUG, "Trade State = %d", TradeState);
		registerTOrderDefOBVs( (IXingAPI*)xingAPIins );
		TradeState = STE_OBVST_INITED;
		break;
	case STE_OBVST_INITED:
	case STE_OBVST_ORDERED:
		ESLOG(LOG_DEBUG, "Trade State = %d", TradeState);
		if (registerTOrderStockOBV(stockCode) == false)
		{
			ESLOG(LOG_ERROR, "registerTOrderStockOBV() is failed.");
			res = false;
			break;
		}
		TradeState = STE_OBVST_ORDERED;
		break;
	case STE_OBVST_DEINITING:
		ESLOG(LOG_ERROR, "Not allowed registering for Trade-Order to Sock-Server. please check your system.");
		res = false;
		break;
	default :
		ESLOG(LOG_DEBUG, "Not Support Trade State = %d", TradeState);
		res = false;
		break;
	}
	pthread_mutex_unlock(&Mutex_TradeState);

	return res;
}

bool CSTReqResManager::unRegisterObservers(STE_OBVORSTATE_TRADE targetState, const char* stockCode)
{
	bool res = true;

	if (TradeState == targetState)
	{
		ESLOG(LOG_INFO, "Already done State(%d).", TradeState);
		return true;
	}

	pthread_mutex_lock(&Mutex_TradeState);
	switch (targetState)
	{
	case STE_OBVST_NONE:	// remain state & remove only stockCode-Observor.
		ESLOG(LOG_ERROR, "target OBVT State = STE_OBVST_NONE, Cur-State = %d", TradeState);
		if (TradeState == STE_OBVST_ORDERED)
		{
			if (unregisterTOrderStockOBV(false, stockCode) == false)
			{
				ESLOG(LOG_ERROR, "Can not unregister Observor.(stockCode=%s)", stockCode);
				res = false;
			}
		}
		break;
	case STE_OBVST_DEINITED:	// Remove All Observors.
		ESLOG(LOG_ERROR, "target OBVT State = STE_OBVST_DEINITED, Cur-State = %d", TradeState);
		if (TradeState == STE_OBVST_ORDERED)
		{
			if (unregisterTOrderStockOBV(true, NULL) == false)
			{
				ESLOG(LOG_ERROR, "Can not unregister Observor.(stockCode=%s)", stockCode);
				res = false;
			}
			TradeState = STE_OBVST_INITED;
		}

		if (TradeState = STE_OBVST_INITED)
		{
			unregisterTOrderDefOBVs((IXingAPI*)xingAPIins);
			TradeState = STE_OBVST_DEINITED;
		}
		break;
	case STE_OBVST_INITED:	// Remove All of stockCodeList-Observors.
		ESLOG(LOG_ERROR, "target OBVT State = STE_OBVST_INITED, Cur-State = %d", TradeState);
		if (TradeState == STE_OBVST_ORDERED)
		{
			if (unregisterTOrderStockOBV(true, NULL) == false)
			{
				ESLOG(LOG_ERROR, "Can not unregister Observor.(stockCode=%s)", stockCode);
				res = false;
			}
			TradeState = STE_OBVST_INITED;
		}
		break;
	default :
		ESLOG(LOG_ERROR, "Not Support target-State = %d", targetState);
		res = false;
		break;
	}
	pthread_mutex_unlock(&Mutex_TradeState);

	return res;
}

bool CSTReqResManager::registerTOrderStockOBV(const char* stockCode)
{
	bool res = true;

	if (stockCode == NULL)
	{
		ESLOG(LOG_ERROR, "Invalid Arguments.");
		res = false;
		return res;
	}

	pthread_mutex_lock(&Mutex_StockOBVMap);
	if (handlerStockOBVMap.find(stockCode) == handlerStockOBVMap.end())
	{
		if (Def_RegiUnregiTOrderStockOBV((IXingAPI*)xingAPIins, true, stockCode) == 1)
			handlerStockOBVMap[stockCode] = (handlerGFuncType)Def_RegiUnregiTOrderStockOBV;
		else
		{
			ESLOG(LOG_ERROR, "Can not register Observor.");
			res = false;
		}
	}
	else
	{
		ESLOG(LOG_INFO, "Already registered Observor.(stockCode=%s)", stockCode);
		res = false;
	}
	pthread_mutex_unlock(&Mutex_StockOBVMap);

	return res;
}

bool CSTReqResManager::unregisterTOrderStockOBV(bool isAllOBVs, const char* stockCode)
{
	bool res = true;
	TOrderMapType::iterator itor;

	if (isAllOBVs == false && stockCode == NULL)
	{
		ESLOG(LOG_ERROR, "Invalid Arguments.");
		res = false;
		return res;
	}

	pthread_mutex_lock(&Mutex_StockOBVMap);
	if (isAllOBVs == false)		// only remove stockCode in Map.
	{
		ESLOG(LOG_DEBUG, "Enter only one Observor removing.");

		if ((itor = handlerStockOBVMap.find(stockCode)) != handlerStockOBVMap.end() && itor->second != NULL)
		{
			if (itor->second(xingAPIins, false, (void*)stockCode) == 1)
			{
				handlerStockOBVMap.erase(itor);
			}
			else
			{
				ESLOG(LOG_ERROR, "Can not unregister Observor.(stockCode=%s)", stockCode);
				res = false;
			}
		}
		else if (itor == handlerStockOBVMap.end())
		{
			ESLOG(LOG_INFO, "Not exist stockCode in map.(stockCode=%s)", stockCode);
		}
		else
		{
			ESLOG(LOG_ERROR, "handler Function is not exist.(dangling pointer)(stockCode=%s)", stockCode);
			res = false;
		}
	}
	else	// remove all stockCode in Map
	{
		ESLOG(LOG_DEBUG, "Enter all Observor removing.");
		for (itor = handlerStockOBVMap.begin(); itor != handlerStockOBVMap.end();)
		{
			if (itor->second == NULL)
			{
				ESLOG(LOG_ERROR, "handler Function is not exist.(stockCode=%s)", itor->first.c_str());
				handlerStockOBVMap.erase(itor++);
				continue;
			}

			if (itor->second(xingAPIins, false, (void*)(itor->first.c_str())) == 1)
			{
				handlerStockOBVMap.erase(itor++);
			}
			else
			{
				ESLOG(LOG_ERROR, "Can not unregister Observor.(stockCode=%s)", itor->first.c_str());
				itor++;
				res = false;
			}
		}
	}
	pthread_mutex_unlock(&Mutex_StockOBVMap);

	return res;
}




/*********************************************************************************************************
*
*	[시작] Received Real Data Event를 위해서 송/수신된 데이터를 처리하는 request/handler들을 나열한 것이다.
*
*********************************************************************************************************/
int CSTReqResManager::handleDummy(void* data, int dataSize)
{
	ESLOG(LOG_DEBUG, "Enter.");

	ESLOG(LOG_DEBUG, "data size = %d", dataSize);
	ESLOG_HEX(LOG_DEBUG, data, dataSize);

	ESLOG(LOG_DEBUG, "Exit.");
	return 1;
}

int CSTReqResManager::handleTOrdSuccess(STOrderSuccessOut* data, int dataSize)
{
	ESLOG(LOG_DEBUG, "Enter.");

	char* stockCode = NULL;

	ESLOG(LOG_DEBUG, "data size = %d, DataType size = %d", dataSize, sizeof(STOrderSuccessOut));
	ESLOG_HEX(LOG_DEBUG, data, dataSize);

	// 주문 처리 결과가 나왔으므로 걸어놓았던, Observor를 unregister한다.
	if ((stockCode = GetCloneStringData(data->Isuno, sizeof(data->Isuno))) == NULL)
	{
		ESLOG(LOG_ERROR, "GetCloneStringData() is failed.");
		return -1;
	}

	ESLOG(LOG_INFO, "Trade Order is Successed. So, Unregister stock-Observor.");
	unRegisterObservers(STE_OBVST_NONE, stockCode);

	ESLOG(LOG_DEBUG, "Exit.");

	return 1;
}

int CSTReqResManager::handleTOrdChanged(STOrderChangedOut* data, int dataSize)
{
	ESLOG(LOG_DEBUG, "Enter.");

	//char* stockCode = NULL;

	ESLOG(LOG_DEBUG, "data size = %d, DataType size = %d", dataSize, sizeof(STOrderChangedOut));
	ESLOG_HEX(LOG_DEBUG, data, dataSize);

	//// 주문 처리 결과가 나왔으므로 걸어놓았던, Observor를 unregister한다.
	//if ((stockCode = GetCloneStringData(data->Isuno, sizeof(data->Isuno))) == NULL)
	//{
	//	ESLOG(LOG_ERROR, "GetCloneStringData() is failed.");
	//	return -1;
	//}

	//ESLOG(LOG_INFO, "Trade Order is Changed. So, Unregister stock-Observor.");
	//unRegisterObservers(STE_OBVST_NONE, stockCode);

	ESLOG(LOG_DEBUG, "Exit.");

	return 1;
}

int CSTReqResManager::handleTOrdCancel(STOrderCancelOut* data, int dataSize)
{
	ESLOG(LOG_DEBUG, "Enter.");

	char* stockCode = NULL;

	ESLOG(LOG_DEBUG, "data size = %d, DataType size = %d", dataSize, sizeof(STOrderCancelOut));
	ESLOG_HEX(LOG_DEBUG, data, dataSize);

	// 주문 처리 결과가 나왔으므로 걸어놓았던, Observor를 unregister한다.
	if ((stockCode = GetCloneStringData(data->Isuno, sizeof(data->Isuno))) == NULL)
	{
		ESLOG(LOG_ERROR, "GetCloneStringData() is failed.");
		return -1;
	}

	ESLOG(LOG_INFO, "Trade Order is Cancel. So, Unregister stock-Observor.");
	unRegisterObservers(STE_OBVST_NONE, stockCode);

	ESLOG(LOG_DEBUG, "Exit.");

	return 1;
}

int CSTReqResManager::handleTOrdRejected(STOrderRejectedOut* data, int dataSize)
{
	ESLOG(LOG_DEBUG, "Enter.");

	char* stockCode = NULL;

	ESLOG(LOG_DEBUG, "data size = %d, DataType size = %d", dataSize, sizeof(STOrderRejectedOut));
	ESLOG_HEX(LOG_DEBUG, data, dataSize);

	// 주문 처리 결과가 나왔으므로 걸어놓았던, Observor를 unregister한다.
	if ((stockCode = GetCloneStringData(data->Isuno, sizeof(data->Isuno))) == NULL)
	{
		ESLOG(LOG_ERROR, "GetCloneStringData() is failed.");
		return -1;
	}

	ESLOG(LOG_INFO, "Trade Order is Rejected. So, Unregister stock-Observor.");
	unRegisterObservers(STE_OBVST_NONE, stockCode);

	ESLOG(LOG_DEBUG, "Exit.");

	return 1;
}

int CSTReqResManager::handleKPISuccess(STkospiSuccessOut* data, int dataSize)
{
	ESLOG(LOG_DEBUG, "Enter.");

	char* stockCode = NULL;

	ESLOG(LOG_DEBUG, "data size = %d, DataType size = %d", dataSize, sizeof(STkospiSuccessOut));
	ESLOG_HEX(LOG_DEBUG, data, dataSize);

	// 주문 처리 결과가 나왔으므로 걸어놓았던, Observor를 unregister한다.
	if ((stockCode = GetCloneStringData(data->shcode, sizeof(data->shcode))) == NULL)
	{
		ESLOG(LOG_ERROR, "GetCloneStringData() is failed.");
		return -1;
	}

	ESLOG(LOG_INFO, "Trade Order(KOSPI) is Successed. So, Unregister stock-Observor.");
	unRegisterObservers(STE_OBVST_NONE, stockCode);

	ESLOG(LOG_DEBUG, "Exit.");

	return 1;
}

int CSTReqResManager::handleKDAQSuccess(STkosdaqSuccessOut* data, int dataSize)
{
	ESLOG(LOG_DEBUG, "Enter.");

	char* stockCode = NULL;

	ESLOG(LOG_DEBUG, "data size = %d, DataType size = %d", dataSize, sizeof(STkosdaqSuccessOut));
	ESLOG_HEX(LOG_DEBUG, data, dataSize);

	// 주문 처리 결과가 나왔으므로 걸어놓았던, Observor를 unregister한다.
	if ((stockCode = GetCloneStringData(data->shcode, sizeof(data->shcode))) == NULL)
	{
		ESLOG(LOG_ERROR, "GetCloneStringData() is failed.");
		return -1;
	}

	ESLOG(LOG_INFO, "Trade Order(KOSDAQ) is Successed. So, Unregister stock-Observor.");
	unRegisterObservers(STE_OBVST_NONE, stockCode);

	ESLOG(LOG_DEBUG, "Exit.");

	return 1;
}

int CSTReqResManager::handleKPIRemain(STkospiRemainOrderOut* data, int dataSize)
{
	ESLOG(LOG_DEBUG, "Enter.");

	ESLOG(LOG_INFO, "Notify remained-Stock occured by Trade Order(KOSPI).");

	//char* stockCode = NULL;

	ESLOG(LOG_DEBUG, "data size = %d, DataType size = %d", dataSize, sizeof(STkospiRemainOrderOut));
	ESLOG_HEX(LOG_DEBUG, data, dataSize);

	//// 주문 처리 결과가 나왔으므로 걸어놓았던, Observor를 unregister한다.
	//if ((stockCode = GetCloneStringData(data->shcode, sizeof(data->shcode))) == NULL)
	//{
	//	ESLOG(LOG_ERROR, "GetCloneStringData() is failed.");
	//	return -1;
	//}

	//unRegisterObservers(STE_OBVST_NONE, stockCode);

	ESLOG(LOG_DEBUG, "Exit.");

	return 1;
}

int CSTReqResManager::handleKDAQRemain(STkosdaqRemainOrderOut* data, int dataSize)
{
	ESLOG(LOG_DEBUG, "Enter.");

	ESLOG(LOG_INFO, "Notify remained-Stock occured by Trade Order(KOSDAQ).");

	//char* stockCode = NULL;

	ESLOG(LOG_DEBUG, "data size = %d, DataType size = %d", dataSize, sizeof(STkosdaqRemainOrderOut));
	ESLOG_HEX(LOG_DEBUG, data, dataSize);

	//// 주문 처리 결과가 나왔으므로 걸어놓았던, Observor를 unregister한다.
	//if ((stockCode = GetCloneStringData(data->shcode, sizeof(data->shcode))) == NULL)
	//{
	//	ESLOG(LOG_ERROR, "GetCloneStringData() is failed.");
	//	return -1;
	//}

	//unRegisterObservers(STE_OBVST_NONE, stockCode);

	ESLOG(LOG_DEBUG, "Exit.");

	return 1;
}


/*********************************************************************************************************
*
*	[끝] Received Real Data Event를 위해서 송/수신된 데이터를 처리하는 request/handler들을 나열한 것이다.
*
*********************************************************************************************************/




/**************************************************************
 *
 *	Internal Function Definition.
 *
 **************************************************************/
static inline void registerTOrderDefOBVs(IXingAPI* xingAPI)
{
	ESLOG(LOG_DEBUG, "Enter.");

	xingAPI->AdviseRealData(TRCODE_ORDER_ACT, "", 0);		// 주식주문접수
	xingAPI->AdviseRealData(TRCODE_ORDER_SUCCESS, "", 0);	// 주식체결성공
	xingAPI->AdviseRealData(TRCODE_ORDER_CHANGED, "", 0);	// 주식주문변경
	xingAPI->AdviseRealData(TRCODE_ORDER_CANCEL, "", 0);	// 주식주문취소
	xingAPI->AdviseRealData(TRCODE_ORDER_REJECTED, "", 0);	// 주식주문거절

	ESLOG(LOG_DEBUG, "Exit.");
}

static inline void unregisterTOrderDefOBVs(IXingAPI* xingAPI)
{
	ESLOG(LOG_DEBUG, "Enter.");

	xingAPI->UnadviseRealData(TRCODE_ORDER_ACT, "", 0);			// 주식주문접수
	xingAPI->UnadviseRealData(TRCODE_ORDER_SUCCESS, "", 0);		// 주식체결성공
	xingAPI->UnadviseRealData(TRCODE_ORDER_CHANGED, "", 0);		// 주식주문변경
	xingAPI->UnadviseRealData(TRCODE_ORDER_CANCEL, "", 0);		// 주식주문취소
	xingAPI->UnadviseRealData(TRCODE_ORDER_REJECTED, "", 0);	// 주식주문거절

	ESLOG(LOG_DEBUG, "Exit.");
}

static int Def_RegiUnregiTOrderStockOBV(IXingAPI* xingAPI, bool isRegister, const char* stockCode)
{
	ESLOG(LOG_DEBUG, "Enter. (isRegister=%d, stockCode=%s)", isRegister, stockCode);

	if (xingAPI == NULL || stockCode == NULL)
	{
		ESLOG(LOG_ERROR, "Invalid Input arguments.");
		return -1;
	}

	int stockLength = strlen(stockCode);

	if (stockLength == 0)
	{
		ESLOG(LOG_ERROR, "stockLength is 0.");
		return -1;
	}

	if (stockLength > 6)
	{
		stockCode = stockCode + (stockLength - 6);
		stockLength = 6;
	}

	if (isRegister == true)	// Register Observor.
	{
		xingAPI->AdviseRealData(TRCODE_KOSPI_SUCCESS, stockCode, stockLength);
		xingAPI->AdviseRealData(TRCODE_KOSDAQ_SUCCESS, stockCode, stockLength);
		xingAPI->AdviseRealData(TRCODE_KOSPI_REMAIN, stockCode, stockLength);
		xingAPI->AdviseRealData(TRCODE_KOSDAQ_REMAIN, stockCode, stockLength);
	}
	else	// Un-Register Observor.
	{
		xingAPI->UnadviseRealData(TRCODE_KOSPI_SUCCESS, stockCode, stockLength);
		xingAPI->UnadviseRealData(TRCODE_KOSDAQ_SUCCESS, stockCode, stockLength);
		xingAPI->UnadviseRealData(TRCODE_KOSPI_REMAIN, stockCode, stockLength);
		xingAPI->UnadviseRealData(TRCODE_KOSDAQ_REMAIN, stockCode, stockLength);
	}

	ESLOG(LOG_DEBUG, "Exit.");
	return 1;
}

//static char* GetChartIndexName(STE_INDEXNAME graph)
//{
//	switch (graph)
//	{
//	case STE_IDXN_DEFAULT:
//		return "가격 이동평균";
//	case STE_IDXN_PRICE_GRAPH:
//		return "가격 이동평균";
//	case STE_IDXN_TRADE_AMOUNT:
//		return "거래량";
//	case STE_IDXN_RSI:
//		return "RSI";
//	default:
//		ESLOG(LOG_DEBUG, "NOT Support Graph IndexName.(%d)", (int)graph);
//		break;
//	}
//
//	return NULL;
//}

#define MAX_QRTCNT		500
int DayOfMon[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
                //  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12

static sREQ* CreateChartRequestData(const char* stockcode, STE_PERIOD period,
	int sYear, int sMon, int sDay, int eYear, int eMon, int eDay,
	int sHour, int sMin, int sSec, int eHour, int eMin, int eSec )
{
	ESLOG(LOG_DEBUG, "Enter.");

	char cPeriod = NULL;
	int   Qrycnt = 0;	// 0 ~ 500 필요한 data 건수.
	int   Ncnt = 0;		// 0 ~ 9999 tick/min 단위
	int   dYear = eYear - sYear;
	int   dMon = eMon - sMon;
	int   dDay = eDay - sDay;
	long  dayCnt = 0;
	int   sTimeCnt = 0;
	int   eTimeCnt = 0;
	sREQ* request = NULL;

	ESLOG(LOG_DEBUG, "Dtart Date=%d-%d-%d, End Date=%d-%d-%d", sYear, sMon, sDay, eYear, eMon, eDay);

	if (stockcode == NULL || strlen(stockcode) == 0 || period <= STE_PRID_SOF || STE_PRID_EOF <= period || dYear < 0)
	{
		ESLOG(LOG_ERROR, "Invalid arguments.(stockcode=%s, period=%d, dYear=%d)", stockcode, period, dYear);
		return NULL;
	}
	
	// Calculation of Month Count.
	if (dMon < 0)
	{
		dYear--;
		dMon = 12 + dMon;
	}

	// Calculation of Day Count.
	if (dDay < 0)
	{
		dMon--;
		dDay = DayOfMon[sMon] - sDay + eDay;

		for (int i=1; i <= dMon; i++)
			dayCnt += DayOfMon[sMon+i];
	}
	else
	{
		for (int i = 0; i < dMon; i++)
			dayCnt += DayOfMon[sMon+i];
	}

	// Error Check
	if (dYear < 0 || dMon < 0 || dDay < 0)
	{
		ESLOG(LOG_ERROR, "Invalid arguments.(dYear=%d, dMon=%d, dDay=%d)", dYear, dMon, dDay);
		return NULL;
	}

	sTimeCnt = sHour * 3600 + sMin * 60 + sSec;
	eTimeCnt = eHour * 3600 + eMin * 60 + eSec;

	// Total Day Count.
	dayCnt = dYear * 365 + dayCnt + dDay;
	ESLOG(LOG_DEBUG, "Total Day Count : %d", dayCnt);

	switch (period)
	{
	case STE_PRID_1TICK:
		cPeriod = '0';
		Ncnt = 1;			// 1 Event per 1 Tick count
		break;
	case STE_PRID_10TICK:
		cPeriod = '0';
		Ncnt = 10;			// 1 Event per 10 Tick count
		break;
	case STE_PRID_100TICK:
		cPeriod = '0';
		Ncnt = 100;			// 1 Event per 100 Tick count
		break;
	case STE_PRID_1MIN:
		cPeriod = '1';
		Ncnt = 1;			// 1 Event per 1 Minite count
		break;
	case STE_PRID_10MIN:
		cPeriod = '1';
		Ncnt = 10;			// 1 Event per 10 Minite count
		break;
	case STE_PRID_30MIN:
		cPeriod = '1';
		Ncnt = 30;			// 1 Event per 30 Minite count
		break;
	case STE_PRID_1HOUR:
		cPeriod = '1';
		Ncnt = 60;			// 1 Event per 60 Minite count
		break;
	case STE_PRID_3HOUR:
		cPeriod = '1';
		Ncnt = 180;			// 1 Event per 180 Minite count
		break;
	case STE_PRID_1DAY:
		cPeriod = '2';
		Ncnt = 0;
		break;
	case STE_PRID_2DAY:
		cPeriod = '2';
		Ncnt = 0;
		break;
	case STE_PRID_10DAY:
		cPeriod = '2';
		Ncnt = 0;
		break;
	case STE_PRID_1WEEK:
		cPeriod = '3';
		Ncnt = 0;
		break;
	case STE_PRID_2WEEK:
		cPeriod = '3';
		Ncnt = 0;
		break;
	case STE_PRID_3WEEK:
		cPeriod = '3';
		Ncnt = 0;
		break;
	case STE_PRID_1MON:
		cPeriod = '4';
		Ncnt = 0;
		break;
	case STE_PRID_3MON:
		cPeriod = '4';
		Ncnt = 0;
		break;
	case STE_PRID_6MON:
		cPeriod = '4';
		Ncnt = 0;
		break;
	case STE_PRID_1YEAR:
		cPeriod = '4';
		Ncnt = 0;
		break;
	default:
		ESLOG(LOG_ERROR, "Not Support period Type.(%d)", period);
		return NULL;
	}

	Qrycnt = MAX_QRTCNT;
	ESLOG(LOG_DEBUG, "cPeriod=%c, Ncnt=%d, Qrycnt=%d", cPeriod, Ncnt, Qrycnt);

	if ((request = (sREQ*)new sREQ()) == NULL)
	{
		ESLOG(LOG_ERROR, "memory allocation is failed.(sREQ)");
		return NULL;
	}

	switch (cPeriod)
	{
	case '0':
	{
		ESLOG(LOG_DEBUG, "cPeriod \"0\"");
		if (sTimeCnt >= eTimeCnt)
		{
			ESLOG(LOG_ERROR, "Invalid argument.(sTimeSec=%d, eTimeSec)", sTimeCnt, eTimeCnt);
			delete request;
			return NULL;
		}

		STChartTickSearchIn* req = NULL;
		if ((req = (STChartTickSearchIn*)ESmalloc(sizeof(STChartTickSearchIn))) == NULL)
		{
			ESLOG(LOG_ERROR, "memory allocation is failed.");
			delete request;
			return NULL;
		}

		CloneValueString(req->sdate, sYear * 10000 + sMon * 100 + sDay, sizeof(req->sdate));
		CloneValueString(req->edate, eYear * 10000 + eMon * 100 + eDay, sizeof(req->edate));
		ESLOG_STRNONULL(LOG_DEBUG, "Start Date=", req->sdate, sizeof(req->sdate));
		ESLOG_STRNONULL(LOG_DEBUG, "End   Date=", req->edate, sizeof(req->edate));

		CloneValueString(req->stime, sHour * 10000 + sMin * 100 + sSec, sizeof(req->stime));
		CloneValueString(req->etime, eHour * 10000 + eMin * 100 + eSec, sizeof(req->etime));
		ESLOG_STRNONULL(LOG_DEBUG, "Start Time=", req->stime, sizeof(req->stime));
		ESLOG_STRNONULL(LOG_DEBUG, "End   Time=", req->etime, sizeof(req->etime));

		CloneValueString(req->shcode, stockcode, sizeof(req->shcode));
		CloneValueString(req->ncnt, Ncnt, sizeof(req->ncnt));
		CloneValueString(req->qrycnt, Qrycnt, sizeof(req->qrycnt));
		CloneValueString(req->nday, (char)'0', sizeof(req->nday));

		CloneValueString(req->cts_date, (char)' ', sizeof(req->cts_date));
		CloneValueString(req->cts_time, (char)' ', sizeof(req->cts_time));
		CloneValueString(req->comp_yn, (char)'N', sizeof(req->comp_yn));

		request->SetRequestStruct(0, REQ_ST_READY, TRCODE_CHART_TICKSEARCH, req, sizeof(STChartTickSearchIn), 1, STChartTickSearchOut1Name);
	}
		break;
	case '1':
	{
		ESLOG(LOG_DEBUG, "cPeriod \"1\"");
		if (sTimeCnt >= eTimeCnt)
		{
			ESLOG(LOG_ERROR, "Invalid argument.(sTimeSec=%d, eTimeSec)", sTimeCnt, eTimeCnt);
			delete request;
			return NULL;
		}

		STChartMinSearchIn* req = NULL;
		if ((req = (STChartMinSearchIn*)ESmalloc(sizeof(STChartMinSearchIn))) == NULL)
		{
			ESLOG(LOG_ERROR, "memory allocation is failed.");
			delete request;
			return NULL;
		}

		CloneValueString(req->sdate, sYear * 10000 + sMon * 100 + sDay, sizeof(req->sdate));
		CloneValueString(req->edate, eYear * 10000 + eMon * 100 + eDay, sizeof(req->edate));
		ESLOG_STRNONULL(LOG_DEBUG, "Start Date=", req->sdate, sizeof(req->sdate));
		ESLOG_STRNONULL(LOG_DEBUG, "End   Date=", req->edate, sizeof(req->edate));

		CloneValueString(req->stime, sHour * 10000 + sMin * 100 + sSec, sizeof(req->stime));
		CloneValueString(req->etime, eHour * 10000 + eMin * 100 + eSec, sizeof(req->etime));
		ESLOG_STRNONULL(LOG_DEBUG, "Start Time=", req->stime, sizeof(req->stime));
		ESLOG_STRNONULL(LOG_DEBUG, "End   Time=", req->etime, sizeof(req->etime));

		CloneValueString(req->shcode, stockcode, sizeof(req->shcode));
		CloneValueString(req->ncnt, Ncnt, sizeof(req->ncnt));
		CloneValueString(req->qrycnt, Qrycnt, sizeof(req->qrycnt));
		CloneValueString(req->nday, (char)'0', sizeof(req->nday));

		CloneValueString(req->cts_date, (char)' ', sizeof(req->cts_date));
		CloneValueString(req->cts_time, (char)' ', sizeof(req->cts_time));
		CloneValueString(req->comp_yn, (char)'N', sizeof(req->comp_yn));

		request->SetRequestStruct(0, REQ_ST_READY, TRCODE_CHART_MINSEARCH, req, sizeof(STChartMinSearchIn), 1, STChartMinSearchOut1Name);
	}
		break;
	case '2':
	case '3':
	case '4':
	{
		ESLOG(LOG_DEBUG, "cPeriod \"%c\"", cPeriod);
		STChartDaysSearchIn* req = NULL;
		if ((req = (STChartDaysSearchIn*)ESmalloc(sizeof(STChartDaysSearchIn))) == NULL)
		{
			ESLOG(LOG_ERROR, "memory allocation is failed.");
			delete request;
			return NULL;
		}

		CloneValueString(req->sdate, sYear * 10000 + sMon * 100 + sDay, sizeof(req->sdate));
		CloneValueString(req->edate, eYear * 10000 + eMon * 100 + eDay, sizeof(req->edate));
		ESLOG_STRNONULL(LOG_DEBUG, "Start Date=", req->sdate, sizeof(req->sdate));
		ESLOG_STRNONULL(LOG_DEBUG, "End   Date=", req->edate, sizeof(req->edate));

		CloneValueString(req->shcode, stockcode, sizeof(req->shcode));
		CloneValueString(req->gubun, cPeriod, sizeof(req->gubun));
		CloneValueString(req->qrycnt, Qrycnt, sizeof(req->qrycnt));

		CloneValueString(req->cts_date, (char)' ', sizeof(req->cts_date));
		CloneValueString(req->comp_yn, (char)'N', sizeof(req->comp_yn));

		request->SetRequestStruct(0, REQ_ST_READY, TRCODE_CHART_DAYSSEARCH, req, sizeof(STChartDaysSearchIn), 1, STChartDaysSearchOut1Name);
	}
		break;
	default :
		ESLOG(LOG_ERROR, "Not Surpport cPeriod \"%c\"", cPeriod);
		delete request;
		request = NULL;
	}
	
	if (request)
	{
		ESLOG(LOG_DEBUG, "Success");
		GetSystemTime(&request->sendTime);
	}

	ESLOG(LOG_DEBUG, "Exit.");
	return request;
}

/***********************************************************
*
* return Type
*  1: des > src
*  0: des == src
* -1: des < src
*
***********************************************************/
static int compareSysTime(WORD src, WORD des)
{
	int res = 0;

	if (src < des)
		res = 1;
	else if (src > des)
		res = -1;

	return res;
}

static int compareSystemTime(SYSTEMTIME& src, SYSTEMTIME& des)
{
	int res = 0;

	if ((res = compareSysTime(src.wYear, des.wYear)) != 0)
		goto GOTO_OUT;
	if ((res = compareSysTime(src.wMonth, des.wMonth)) != 0)
		goto GOTO_OUT;
	if ((res = compareSysTime(src.wDay, des.wDay)) != 0)
		goto GOTO_OUT;
	if ((res = compareSysTime(src.wHour, des.wHour)) != 0)
		goto GOTO_OUT;
	if ((res = compareSysTime(src.wMinute, des.wMinute)) != 0)
		goto GOTO_OUT;
	if ((res = compareSysTime(src.wSecond, des.wSecond)) != 0)
		goto GOTO_OUT;

	res = compareSysTime(src.wMilliseconds, des.wMilliseconds);

GOTO_OUT:
	return res;
}

static bool CheckInputData4ChartSearch(void* data, int dataSize, sREQ* req, int DataUnitSize)
{
	if (data == NULL || req == NULL || dataSize == 0)
	{
		ESLOG(LOG_ERROR, "Invalid argument.(data=0x%X, size=%d, req=0x%X)", data, dataSize, req);
		return false;
	}

	if (req->RepRcdCnt == 0)
	{
		ESLOG(LOG_ERROR, "Invalid Record Count.(NULL)");
		return false;
	}

	ESLOG(LOG_DEBUG, "recoard Count=%d", req->RepRcdCnt);
	ESLOG(LOG_DEBUG, "dataSize=%d, ExpDataSize=%d", dataSize, (DataUnitSize*req->RepRcdCnt));
	if (dataSize != (DataUnitSize*req->RepRcdCnt))
	{
		ESLOG(LOG_ERROR, "Invalid Record Count.(dataSize=%d)", dataSize);
		return false;
	}

	return true;
}

static void hadleCRecordTickSearchData( STChartTickSearchOut2* data, int dataSize, sREQ* req)
{
	ESLOG(LOG_DEBUG, "Enter.");

	int i = 0;
	if (CheckInputData4ChartSearch(data, dataSize, req, sizeof(STChartTickSearchOut2)) == false)
	{
		ESLOG(LOG_ERROR, "CheckInputData4ChartSearch is failed.");
		return;
	}

	while (i < req->RepRcdCnt)
	{
		char* date = NULL;

		date = GetCloneStringData(data[i].date, sizeof(data[i].date));
		ESLOG(LOG_DEBUG, "date = %s", date);
		ESfree(date);
		date = NULL;

		i++;
	}

	ESLOG(LOG_DEBUG, "Exit.");
}

static void hadleCRecordMinSearchData(STChartMinSearchOut2* data, int dataSize, sREQ* req)
{
	ESLOG(LOG_DEBUG, "Enter.");

	int i = 0;
	if (CheckInputData4ChartSearch(data, dataSize, req, sizeof(STChartMinSearchOut2)) == false)
	{
		ESLOG(LOG_ERROR, "CheckInputData4ChartSearch is failed.");
		return;
	}

	while (i < req->RepRcdCnt)
	{
		char* date = NULL;

		date = GetCloneStringData(data[i].date, sizeof(data[i].date));
		ESLOG(LOG_DEBUG, "date = %s", date);
		ESfree(date);
		date = NULL;

		i++;
	}

	ESLOG(LOG_DEBUG, "Exit.");
}

static void hadleCRecordDaysSearchData(STChartDaysSearchOut2* data, int dataSize, sREQ* req)
{
	ESLOG(LOG_DEBUG, "Enter.");

	int i = 0;
	if (CheckInputData4ChartSearch(data, dataSize, req, sizeof(STChartDaysSearchOut2)) == false)
	{
		ESLOG(LOG_ERROR, "CheckInputData4ChartSearch is failed.");
		return;
	}

	while (i < req->RepRcdCnt)
	{
		char* date = NULL;

		date = GetCloneStringData(data[i].date, sizeof(data[i].date));
		ESLOG(LOG_DEBUG, "date = %s", date);
		ESfree(date);
		date = NULL;

		i++;
	}

	ESLOG(LOG_DEBUG, "Exit.");
}
