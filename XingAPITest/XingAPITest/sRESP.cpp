#include "sRESP.h"

#define LOGTAG	"sRESP"
#include "dlogger.h"

#include <Windows.h>
#include "EScommon.h"

#include <ocstack.h>



typedef struct sCMD_DEFINE {
	const char*		name;		// json format���� ����ϴ� command �̸�. (cmd)
	REQ_CMDLIST	cmdIdx;			// name�� 1:1�� mapping�Ǵ� cmd�� Type�̴�.
	CMD_TYPE	type;			// name Group���� �̸� ������ command�� type.
	sRESP::HandlerFunc handler;	// command���� ��������� ���ؼ� ���Ǵ� handler.
}sCMD_DEFINE;

// Separator declation for GET/PUT/POST request Parsor
static const char* 	CMDTYPE_GETProgStatus = "GetProgStatus";
static const char* 	CMDTYPE_GETTime = "GetTime";
static const char* 	CMDTYPE_GETStockData = "GetStockData";
static const char* 	CMDTYPE_GETAccountData = "GetActData";
static const char* 	CMDTYPE_GUTOrderAuth = "GutOrderAuth";
static const char* 	CMDTYPE_PUTOrderAuth = "PutOrderAuth";
static const char* 	CMDTYPE_POSTSearchStockList = "SearchStockList";
static const char* 	CMDTYPE_POSTSearchGraph = "SearchGraph";
static const char* 	CMDTYPE_POSTOrderStock = "OrderStock";

static TransactionDataType makeTransNum(void);

static const sCMD_DEFINE CMDs_Array[] = {
	{ CMDTYPE_GETProgStatus		  , GET_PROG_STATUS, CMDTYPE_LOGIC , NULL },
	{ CMDTYPE_GETTime			  , GET_TIME, CMDTYPE_XING , NULL },
	{ CMDTYPE_GETStockData		  , GET_STOCK, CMDTYPE_XING , NULL },
	{ CMDTYPE_GETAccountData	  , GET_ACCOUNT, CMDTYPE_XING , NULL },
	{ CMDTYPE_GUTOrderAuth		  , GUT_ORDERAUTH, CMDTYPE_LOGIC , NULL },
	{ CMDTYPE_PUTOrderAuth		  , PUT_ORDERAUTH, CMDTYPE_LOGIC , NULL },
	{ CMDTYPE_POSTSearchStockList , POST_STOCKLIST, CMDTYPE_XING , NULL },
	{ CMDTYPE_POSTSearchGraph	  , POST_SEARCHGRAPH, CMDTYPE_XING , NULL },
	{ CMDTYPE_POSTOrderStock	  , POST_ORDERSTOCK, CMDTYPE_XING , NULL },
	{ NULL	  , CMDLIST_NULL, CMDTYPE_NULL , NULL },
};





REQ_CMDLIST sRESP::CvtCMDsType(OCRepPayload* data)
{
	ESLOG(LOG_DEBUG, "Enter.");

	int i = 0;
	char* cmdName = NULL;
	REQ_CMDLIST cmdsType = CMDLIST_NULL;

	cmdsMinor = CMDLIST_NULL;

	if (data == NULL)
	{
		ESLOG(LOG_ERROR, "Invalid input data.(data = 0x%X)", data);
		return cmdsType;
	}

	// Major Command�� ã�Ƽ� �ĺ��ϴ� Logic.
	if ((cmdName = parsedData->GetMajorCmd()) == NULL)
	{
		ESLOG(LOG_ERROR, "cmd-Name is NULL.");
		return cmdsType;
	}
	
	i = 0;
	while (CMDs_Array[i].name != NULL && strcmp(cmdName, CMDs_Array[i].name) != 0)
		i++;

	if (CMDs_Array[i].name == NULL)
	{
		ESLOG(LOG_ERROR, "Not-Supported Command Name.(name=%s)", cmdName);
		return cmdsType;
	}

	majorCmd.cmd = CMDs_Array[i].cmdIdx;
	majorCmd.type = CMDs_Array[i].type;
	majorCmd.handlerMakingPayloadResult = CMDs_Array[i].handler;
	ESfree(cmdName);
	cmdName = NULL;
	ESLOG(LOG_DEBUG, "cmd: name=%s, cmdIdx=%d, type=%d, handler=0x%X", CMDs_Array[i].name
																	, majorCmd.cmd
																	, majorCmd.type
																	, majorCmd.handlerMakingPayloadResult);

	// Minor Commands�� ã�Ƽ� �ĺ��ϴ� Logic.
	if ((minorCmdEA = parsedData->GetMinorCmdEA()) > 0)
	{
		if ((minorCmds = new sRESP_Result[minorCmdEA]()) == NULL)
		{
			ESLOG(LOG_ERROR, "minorCmds : memory allocation is failed.");
			return cmdsType;
		}

		for (int cnt = 0; cnt < minorCmdEA; cnt++)
		{
			if ((cmdName = parsedData->GetMinorCmd(cnt)) == NULL)
			{
				ESLOG(LOG_ERROR, "cmd-Name is NULL.");
				return cmdsType;
			}

			i = 0;
			while (CMDs_Array[i].name != NULL && strcmp(cmdName, CMDs_Array[i].name) != 0)
				i++;

			if (CMDs_Array[i].name == NULL)
			{
				ESLOG(LOG_ERROR, "Not-Supported Command Name.(name=%s)", cmdName);
				return cmdsType;
			}

			minorCmds[cnt].cmd = CMDs_Array[i].cmdIdx;
			minorCmds[cnt].type = CMDs_Array[i].type;
			minorCmds[cnt].handlerMakingPayloadResult = CMDs_Array[i].handler;
			ESfree(cmdName);
			cmdName = NULL;

			cmdsMinor = (REQ_CMDLIST)(cmdsMinor | minorCmds[cnt].cmd);

			ESLOG(LOG_DEBUG, "MinorList=0x%X, cmd: name=%s, cmdIdx=0x%X, type=%d, handler=0x%X", cmdsMinor
																		, CMDs_Array[i].name
																		, majorCmd.cmd
																		, majorCmd.type
																		, majorCmd.handlerMakingPayloadResult);
		}
	}

	cmdsType = (REQ_CMDLIST)(majorCmd.cmd | cmdsMinor);
	ESLOG(LOG_DEBUG, "Exit.(cmdsType=0x%X)", cmdsType);

	return cmdsType;
}


sRESP::sRESP(OCEntityHandlerRequest* pReqEntity, OCRepPayload* data)
{
	ESLOG(LOG_DEBUG, "Enter.");

	transID = 0;				// iot-request�� ���� Response������ �ǹ��Ѵ�.
	uri = NULL;					// request�� ���� URI�� �ǹ��Ѵ�.
	cmdsMinor = CMDLIST_NULL;	// requestó���� ���ؼ�, �����ִ� Xing command ó���� ������ ��Ÿ����.
	minorCmdEA = 0;				// minor command ����.
	status = RESP_NONE;			// request�� ���� Response�� ������� ����������, ���¸� ��Ÿ����.

	devAddr = NULL;						// response�ؾ��� ��� client�� �ּҸ� ��Ÿ����.
	requestHandle = NULL;				// request�� ���� handler�� ��Ÿ����.
	resourceHandle= NULL;				// resource�� ���� handler�� ��Ÿ����.
	minorCmds = NULL;

	if (pReqEntity == NULL || data == NULL)
	{
		ESLOG(LOG_ERROR, "Invalid input data.(pReqEntity=0x%X, data=0x%X)", pReqEntity, data);
		return;
	}

	if ((parsedData = new iotReqDecoder((OCPayload*)data)) == NULL)
	{
		ESLOG(LOG_ERROR, "parsedData: memroy allocation is failed.");
		return;
	}

	if (parsedData->GetResultParsing() == false)
	{
		ESLOG(LOG_ERROR, "iotReqDecoder parsing is filed.");
		return ;
	}

	requestHandle = pReqEntity->requestHandle;
	resourceHandle = pReqEntity->resource;
	if (requestHandle == NULL || resourceHandle == NULL)
	{
		ESLOG(LOG_INFO, "requestHandle or resourceHandle is NULL.");
		return;
	}

	ESLOG(LOG_DEBUG, "search commands and convert to CMDLIST from request-payload.");
	if (CvtCMDsType(data) == CMDLIST_NULL)
	{
		ESLOG(LOG_INFO, "Can not convert CMDsType from request-data-payload.");
		return;
	}

	ESLOG(LOG_DEBUG, "uri copy start.");
	if( (uri = ESstrdup(OCGetResourceUri(pReqEntity->resource))) == NULL )
	{
		ESLOG(LOG_ERROR, "Can not read request received-URI.");
		return;
	}
	ESLOG(LOG_DEBUG, "uri=%s", uri);

	ESLOG(LOG_DEBUG, "devAddr copy start.(size=%d)", sizeof(OCDevAddr) );
	if ((devAddr = (OCDevAddr*)ESmemdup(&(pReqEntity->devAddr), sizeof(OCDevAddr))) == NULL)
	{
		ESLOG(LOG_ERROR, "devAddr memory allocation is failed.");
		ESfree(uri);
		uri = NULL;
		return;
	}

	status = RESP_DONE_INIT;
	transID = makeTransNum();
	ESLOG(LOG_DEBUG, "transID=%lld", transID);

	ESLOG(LOG_DEBUG, "Exit. (Success)");
}

sRESP::~sRESP(void)
{
	ESLOG(LOG_DEBUG, "Enter.");

	status = RESP_NONE;
	transID = 0;
	majorCmd.handlerMakingPayloadResult = NULL;

	ESfree(uri);
	uri = NULL;

	ESfree(devAddr);
	devAddr = NULL;

	ESfree(majorCmd.payload);
	majorCmd.payload = NULL;

	if (minorCmds)
	{
		delete minorCmds;
		minorCmds = NULL;
		minorCmdEA = 0;
	}

	if (parsedData)
	{
		delete parsedData;
		parsedData = NULL;
	}

	ESLOG(LOG_DEBUG, "Exit.");
}




/**
* @brief API for getting transaction number
* @param transNum   : transaction Number
*/
TransactionDataType sRESP::GetTransID(void)
{
	return transID;
}

/**
* @brief API for setting frameid
* @brief Frameid will be used to match the response delivered from DA SW with received request
* @param cmdID           : Frame ID
*/
REQ_CMDLIST sRESP::GetMajorCmd(void)
{
	return majorCmd.cmd;
}

REQ_CMDLIST sRESP::GetMinorCmds(void)
{
	return cmdsMinor;
}

/**
* @brief API for getting Device-Address.
*/
const OCDevAddr* sRESP::GetDevAddr(void)
{
	return devAddr;
}

/**
* @brief API for getting URI
* @param char** uri    : reference of the char* variable to where uri will be copied
*/
const char* sRESP::GetUri(void)
{
	return uri;
}

/**
* @brief API for getting URI
* @param char** uri    : reference of the char* variable to where uri will be copied
*/
RESP_STATUS sRESP::GetStatus(void)
{
	return status;
}

void sRESP::SetStatus(RESP_STATUS state)
{
	if (state == RESP_NONE)
		return;

	if (state != RESP_ERROR && state <= status)
	{
		ESLOG(LOG_ERROR, "Not Supported state value.");
		return;
	}

	status = state;
}

/**
* @brief API for getting error when returning request handling result
* @param double        : error code
*/
OCEntityHandlerResult sRESP::GetResult(void)
{
	return majorCmd.result;
}

/**
* @brief API for creating instance of payload (Internal use only, NOT API)
* @return                    : New instance OCRepPayload data structure defined by IoTivity stack
*/
RESP_STATUS sRESP::MakePayloadResult(TransactionDataType transNum, int DoneCmdType, void* rawData, int rawDataSize, HandlerFunc callback)
{
	ESLOG(LOG_DEBUG, "Enter.");

	;

	ESLOG(LOG_DEBUG, "Exit.");
	return RESP_ERROR;
}




/************************************
 * Internal Function Definition.
 */

static TransactionDataType TransactionNum = 0;
static pthread_mutex_t Mutex_TransNum = PTHREAD_MUTEX_INITIALIZER;

static TransactionDataType makeTransNum(void)
{
	ESLOG(LOG_DEBUG, "Enter.");
	TransactionDataType transNum = 0;

	// make a transaction Number per message.
	pthread_mutex_lock(&Mutex_TransNum);
	TransactionNum++;
	transNum = TransactionNum;
	pthread_mutex_unlock(&Mutex_TransNum);

	ESLOG(LOG_DEBUG, "Exit. (transNum=%lld)", transNum);
	return transNum;
}








