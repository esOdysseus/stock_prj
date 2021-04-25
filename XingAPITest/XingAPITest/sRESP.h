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
	CMDTYPE_XING = 1,		// Xing API�� ���ؼ�, �ֽ� Server�� ������ �ʿ��� Cmd��.
	CMDTYPE_LOGIC = 2,		// Program ��ü������ ó���� ������ �ܼ� Cmd��.
	CMDTYPE_MIXED = 3		// Xing Cmd���� Minor command�� �ΰ��ִ� Logical Cmd��.
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
		OCEntityHandlerResult   result;			// request�� ó���� ����� ��Ÿ����.
		OCPayload*				payload;		// request�� ó���� ��� payload�� ��Ÿ����.
		HandlerFunc				handlerMakingPayloadResult;	// request�� SLOW-responseó���� �Ҷ�
															// , ó������� ����� handler �Լ��� ��Ÿ����.

		sRESP_Result(void) {
			cmd = CMDLIST_NULL;
			type = CMDTYPE_NULL;
			result = OC_EH_ERROR;
			payload = NULL;
			handlerMakingPayloadResult = NULL;
		}
	};

	iotReqDecoder*		parsedData;		// request�� parsing�� ������ �����ϴ� ��.

	TransactionDataType transID;		// iot-request�� ���� Response������ �ǹ��Ѵ�.
	char*				uri;			// request�� ���� URI�� �ǹ��Ѵ�.
	REQ_CMDLIST			cmdsMinor;		// requestó���� ���ؼ�, �����ִ� Xing command ó���� ������ ��Ÿ����.
	int					minorCmdEA;		// minor command ����.
	RESP_STATUS			status;			// request�� ���� Response�� ������� ����������, ���¸� ��Ÿ����.

	OCDevAddr*				devAddr;		// response�ؾ��� ��� client�� �ּҸ� ��Ÿ����.
	OCRequestHandle         requestHandle;	// request�� ���� handler�� ��Ÿ����.
	OCResourceHandle        resourceHandle;	// resource�� ���� handler�� ��Ÿ����.
	sRESP_Result			majorCmd;			// major cmd�� ����� �����Ѵ�.
	sRESP_Result*			minorCmds;			// minorCmdEA ��ŭ Array�� ����� �� cmd�� ����� �����Ѵ�.

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