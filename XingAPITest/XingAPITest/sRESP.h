#ifndef _SRESP_H_SUB_CLASS_EUNSEOK_KIM_
#define _SRESP_H_SUB_CLASS_EUNSEOK_KIM_

#include "Config.h"
#include <pthread.h>
#include <octypes.h>

#include "vCIoTREQParsor.h"
#include "IoTreqDecoder.h"

typedef enum
{
	RESP_NONE = 0,
	RESP_DONE_INIT = 1,
	RESP_NEED_MINORCMDS = 2,
	RESP_NEED_MAJORCMD = 3,
	RESP_DONE_ALLCMD = 4,
	RESP_SENT = 5,
	RESP_ERROR = 6,
	RESP_EOF
} RESP_STATUS;

typedef enum
{
	CMDTYPE_NULL = 0,
	CMDTYPE_XING = 1,		// Xing API를 통해서, 주식 Server에 접근이 필요한 Cmd임.
	CMDTYPE_LOGIC = 2,		// Program 자체적으로 처리가 가능한 단순 Cmd임.
	CMDTYPE_MIXED = 3		// Xing Cmd들을 Minor command로 두고있는 Logical Cmd임.
} CMD_TYPE;

class sRESP
{
public:
	typedef RESP_STATUS (*HandlerFunc)(REQ_CMDLIST DoneCmdType, void* rawData, int rawDataSize, OCPayload** pPayload, OCEntityHandlerResult* pResult);
private:
	class sRESP_Result
	{
	public : 
		REQ_CMDLIST				cmd;
		CMD_TYPE				type;
		OCEntityHandlerResult   result;			// request를 처리한 결과를 나타낸다.
		OCPayload*				payload;		// request를 처리한 결과 payload를 나타낸다.
		HandlerFunc				handlerMakingPayloadResult;	// request를 SLOW-response처리를 할때
															// , 처리결과를 만드는 handler 함수를 나타낸다.

		sRESP_Result(void) {
			cmd = CMDLIST_NULL;
			type = CMDTYPE_NULL;
			result = OC_EH_ERROR;
			payload = NULL;
			handlerMakingPayloadResult = NULL;
		}
	};

	iotReqDecoder*		parsedData;		// request를 parsing한 내용을 저장하는 곳.

	TransactionDataType transID;		// iot-request에 대한 Response대응을 의미한다.
	char*				uri;			// request가 들어온 URI를 의미한다.
	REQ_CMDLIST			cmdsMinor;		// request처리를 위해서, 남아있는 Xing command 처리들 내역을 나타낸다.
	int					minorCmdEA;		// minor command 개수.
	RESP_STATUS			status;			// request에 대한 Response가 어느정도 수행중인지, 상태를 나타낸다.

	OCDevAddr*				devAddr;		// response해야할 대상 client의 주소를 나타낸다.
	OCRequestHandle         requestHandle;	// request의 고유 handler를 나타낸다.
	OCResourceHandle        resourceHandle;	// resource의 고유 handler를 나타낸다.
	sRESP_Result			majorCmd;			// major cmd의 결과를 관리한다.
	sRESP_Result*			minorCmds;			// minorCmdEA 만큼 Array로 만들고 각 cmd의 결과를 관리한다.

public:
	sRESP(OCEntityHandlerRequest* pReqEntity, OCRepPayload* data);

	~sRESP(void);

	/**
	* @brief API for getting transaction number
	* @param transNum   : transaction Number
	*/
	TransactionDataType GetTransID(void);

	/**
	* @brief API for setting frameid
	* @brief Frameid will be used to match the response delivered from DA SW with received request
	* @param cmdID           : Frame ID
	*/
	REQ_CMDLIST GetMajorCmd(void);

	REQ_CMDLIST GetMinorCmds(void);

	/**
	* @brief API for getting Device-Address.
	*/
	const OCDevAddr* GetDevAddr(void);

	/**
	* @brief API for getting URI
	* @param char** uri    : reference of the char* variable to where uri will be copied
	*/
	const char* GetUri(void);

	/**
	* @brief API for getting URI
	* @param char** uri    : reference of the char* variable to where uri will be copied
	*/
	RESP_STATUS GetStatus(void);

	/**
	* @brief API for setting Status
	* @param RESP_STATUS state    : to be setting Status value.
	*/
	void SetStatus(RESP_STATUS state);

	/**
	* @brief API for getting error when returning request handling result
	* @param double        : error code
	*/
	OCEntityHandlerResult GetResult(void);

	/**
	* @brief API for creating instance of payload (Internal use only, NOT API)
	* @return                    : New instance OCRepPayload data structure defined by IoTivity stack
	*/
	RESP_STATUS MakePayloadResult(TransactionDataType transNum, int DoneCmdType, void* rawData, int rawDataSize, HandlerFunc callback);

private:
	REQ_CMDLIST CvtCMDsType(OCRepPayload* data);
};



#endif	// _SRESP_H_SUB_CLASS_EUNSEOK_KIM_