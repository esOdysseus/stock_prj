#ifndef __IOTIVITY_RESOURCE_HANDLER_BY_EUNSEOK_KIM__
#define __IOTIVITY_RESOURCE_HANDLER_BY_EUNSEOK_KIM__

#include "Config.h"
#include <pthread.h>

#include <octypes.h>

/* Structure to represent a Stock-request Handler resource */
typedef struct IR_STReqHandle
{
	char* uri;
	char* rt;
	char* inf;
	OCResourceHandle handle;
} IR_STReqHandle;



#include <map>
#include "sRESP.h"

/**
* @brief  Managing-Class both sending request and handling response with regard to Stock.
*/
class CIRscManager 
{
private:
	typedef OCEntityHandlerResult (CIRscManager::*handlerFuncType)(OCEntityHandlerRequest* pReqEntity, void* data);

	typedef class ChandlerCol
	{
	public :
		char* uri;
		handlerFuncType GEThandler;
		handlerFuncType PUThandler;
		handlerFuncType POSThandler;
		handlerFuncType DELETEhandler;

		ChandlerCol(void) : uri(NULL), GEThandler(NULL), PUThandler(NULL), POSThandler(NULL), DELETEhandler(NULL) {};
		void initial(const char* URI, handlerFuncType getHandler, 
			        handlerFuncType putHandler, handlerFuncType postHandler, handlerFuncType deleteHandler);
		~ChandlerCol(void) { free(uri); uri = NULL; GEThandler = NULL; PUThandler = NULL; POSThandler = NULL; DELETEhandler = NULL; };
	} ChandlerCol;

	IR_STReqHandle RscSTreqH;

	std::map<OCResourceHandle, ChandlerCol> handlerMap;
	std::map<TransactionDataType, sRESP*> sRespMap;

public:
	CIRscManager(void);

	CIRscManager(bool* isSuccess);

	~CIRscManager(void);

	OCEntityHandlerResult CallHandler(OCEntityHandlerRequest* pReqEntity);

private:
	OCEntityHandlerResult handleStockRequest(OCEntityHandlerRequest* pReqEntity, OCRepPayload* data);

	OCEntityHandlerResult GEThandleStockRequest(OCEntityHandlerRequest* pReqEntity, OCRepPayload* data);

	OCEntityHandlerResult PUThandleStockRequest(OCEntityHandlerRequest* pReqEntity, OCRepPayload* data);

	OCEntityHandlerResult POSThandleStockRequest(OCEntityHandlerRequest* pReqEntity, OCRepPayload* data);

	OCEntityHandlerResult DELhandleStockRequest(OCEntityHandlerRequest* pReqEntity, OCRepPayload* data);
};


#endif // __IOTIVITY_RESOURCE_HANDLER_BY_EUNSEOK_KIM__

