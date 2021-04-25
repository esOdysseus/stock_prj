#include "Config.h"
#include <Windows.h>
#include <pthread.h>
#include <string.h>

#include "CIRscManager.h"

#define LOGTAG	"CIRscManager"
#include "dlogger.h"

#include "EScommon.h"

// iotivity headers.
#include "IoTivityAPI.h"
#include "iotivity_config.h"
#include "ocpayload.h"
#include "ocstack.h"
#include "vCIoTREQParsor.h"
#include "sRESP.h"

#define IRSC_STREQHANDLE		"/stock/reqHandler"
#define IRSC_STREQRT			"oc.rt.stockReqh"
#define IRSC_STREQINF			"oc.if.rw"

CIRscManager* pInstance = NULL;

static OCResourceHandle CreateIoTivityResource(const char *uri, const char* rtName, const char* infName);
static void PrintRepPayloadValue(OCRepPayloadValue* values);
static OCEntityHandlerResult OCEntityHandlerCb(OCEntityHandlerFlag flag, OCEntityHandlerRequest *pReq, void* callback);

CIRscManager::CIRscManager(void)
{
	ESLOG(LOG_DEBUG, "Enter.");
	pInstance = NULL;

	RscSTreqH.handle = NULL;
	RscSTreqH.uri    = NULL;
	RscSTreqH.rt     = NULL;
	RscSTreqH.inf    = NULL;

	ESLOG(LOG_DEBUG, "Exit.");
}

CIRscManager::CIRscManager(bool* isSuccess)
{
	ESLOG(LOG_DEBUG, "Enter.");

	*isSuccess = false;
	pInstance = NULL;

	RscSTreqH.handle = NULL;
	RscSTreqH.uri = NULL;
	RscSTreqH.rt = NULL;
	RscSTreqH.inf = NULL;

	if ((RscSTreqH.handle = CreateIoTivityResource(IRSC_STREQHANDLE, IRSC_STREQRT, IRSC_STREQINF)) == NULL)
	{
		ESLOG(LOG_ERROR, "Creating Resource \"%s\" is failed.", RscSTreqH.uri);
		return ;
	}
	RscSTreqH.uri = ESstrdup(IRSC_STREQHANDLE);
	RscSTreqH.rt = ESstrdup(IRSC_STREQRT);
	RscSTreqH.inf = ESstrdup(IRSC_STREQINF);

	handlerMap[RscSTreqH.handle].initial(RscSTreqH.uri, 
										 (handlerFuncType)&CIRscManager::handleStockRequest,
										 (handlerFuncType)&CIRscManager::handleStockRequest,
										 (handlerFuncType)&CIRscManager::handleStockRequest,
										 (handlerFuncType)&CIRscManager::DELhandleStockRequest
										);

	pInstance = this;
	*isSuccess = true;

	ESLOG(LOG_DEBUG, "Exit.");
}

CIRscManager::~CIRscManager(void)
{
	ESLOG(LOG_DEBUG, "Enter.");

	// [TODO] sRespMap map delete
	;

	if (OCDeleteResource(RscSTreqH.handle) != OC_STACK_OK)
	{
		ESLOG(LOG_ERROR, "Deleting \"%s\" Resource is failed.", handlerMap[RscSTreqH.handle].uri);
	}

	ESfree(RscSTreqH.uri);
	ESfree(RscSTreqH.rt);
	ESfree(RscSTreqH.inf);
	RscSTreqH.uri = NULL;
	RscSTreqH.rt  = NULL;
	RscSTreqH.inf = NULL;
	RscSTreqH.handle = NULL;

	pInstance = NULL;

	ESLOG(LOG_DEBUG, "Exit.");
}

OCEntityHandlerResult CIRscManager::CallHandler(OCEntityHandlerRequest* pReqEntity)	
{
	handlerFuncType  func = NULL;
	OCEntityHandlerResult res = OC_EH_ERROR;
	OCResourceHandle resource = NULL;
	OCMethod method = OC_REST_NOMETHOD;
	OCRepPayload* payload = NULL;

	if (pInstance == NULL)
	{
		ESLOG(LOG_ERROR, "Not initialized CIRscManager Class.");
		return res;
	}

	if (pReqEntity == NULL)
	{
		ESLOG(LOG_ERROR, "pReqEntity is NULL.");
		return res;
	}

	resource = pReqEntity->resource;
	method = pReqEntity->method;
	payload = (OCRepPayload*)pReqEntity->payload;
	if (resource == NULL || method < OC_REST_GET || method > OC_REST_DELETE )
	{
		ESLOG(LOG_ERROR, "Invalid input arguments.(handle=0x%X, method=%d)", pReqEntity->resource, pReqEntity->method);
		return res;
	}

	switch (method)
	{
	case OC_REST_GET:
		ESLOG(LOG_INFO, "OC_REST_GET");
		func = handlerMap[resource].GEThandler;
		break;
	case OC_REST_PUT:
		ESLOG(LOG_INFO, "OC_REST_PUT");
		func = handlerMap[resource].PUThandler;
		break;
	case OC_REST_POST:
		ESLOG(LOG_INFO, "OC_REST_POST");
		func = handlerMap[resource].POSThandler;
		break;
	case OC_REST_DELETE:
		ESLOG(LOG_INFO, "OC_REST_DELETE");
		func = handlerMap[resource].DELETEhandler;
		break;
	}

	if (func != NULL)
	{
		res = (this->*func)(pReqEntity, payload);

		if (res == OC_EH_OK || res == OC_EH_SLOW)
		{
			ESLOG(LOG_INFO, "Function process Successs.(uri=%s, result=%d)", handlerMap[resource].uri, res);
		}
		else
		{
			ESLOG(LOG_ERROR, "Function process Failed.(uri=%s, result=%d)", handlerMap[resource].uri, res);
		}
	}
	else
	{
		ESLOG(LOG_INFO, "Not Support this method.(uri=%s, method=%d)", handlerMap[resource].uri, method);
		res = OC_EH_NOT_ACCEPTABLE;
	}
	
	return res;
}






/**********************************************************************
 * 
 *  Internal Function Definition.
 *
 **********************************************************************/
OCEntityHandlerResult CIRscManager::handleStockRequest( OCEntityHandlerRequest* pReqEntity, OCRepPayload* data)
{
	ESLOG(LOG_DEBUG, "Enter.");

	sRESP* pIoTResp = NULL;
	vCIoTReqParsor* IoTreqParsorIns = NULL;
	TransactionDataType transNum = 0;
	REQ_CMDLIST method = CMDLIST_NULL;

	if (pInstance == NULL)
	{
		ESLOG(LOG_ERROR, "Not initialized CIRscManager Class.");
		return OC_EH_ERROR;
	}

	if (data == NULL || pReqEntity == NULL)
	{
		ESLOG(LOG_ERROR, "Invalid Input argument.");
		return OC_EH_ERROR;
	}

	PrintRepPayloadValue(data->values);		// for Debugging.
											//OCRepPayloadGetPropInt(input, "power", &pow)

	if ((IoTreqParsorIns = IoTivityAPI::GetInstanceReqParsor()) == NULL)
	{
		ESLOG(LOG_ERROR, "IoTreqParsorIns is NULL.");
		return OC_EH_ERROR;
	}

	// make Resp-structure per message.
	ESLOG(LOG_DEBUG, "Insert to Map a Client-Info in received Request~!!!!");
	if ((pIoTResp = new sRESP(pReqEntity, data)) == NULL )
	{
		ESLOG(LOG_ERROR, "Create sRESP structure is failed.");
		return OC_EH_ERROR;
	}
	
	method = pIoTResp->GetMajorCmd();
	if ((transNum = pIoTResp->GetTransID()) == 0)
	{
		ESLOG(LOG_ERROR, "transaction number has a error.");
		delete pIoTResp;
		pIoTResp = NULL;
		return OC_EH_ERROR;
	}
	sRespMap[transNum] = pIoTResp;

	// it's gerentied thread-safty.
	if (IoTreqParsorIns->IoT_ReqParse(method, data, transNum) == false)
	{
		ESLOG(LOG_ERROR, "Result of IoT_ReqParse()_func is Failed.");
		sRespMap[transNum] = NULL;	// [TODO] need to delete iterator
		delete pIoTResp;
		pIoTResp = NULL;
		return OC_EH_ERROR;
	}

	ESLOG(LOG_DEBUG, "Exit.");
	return OC_EH_SLOW;
}

OCEntityHandlerResult CIRscManager::GEThandleStockRequest(OCEntityHandlerRequest* pReqEntity, OCRepPayload* data)
{
	ESLOG(LOG_DEBUG, "Enter.");

	OCEntityHandlerResult res = OC_EH_ERROR;
	res = handleStockRequest(pReqEntity, data);

	ESLOG(LOG_DEBUG, "Exit.");
	return res;
}

OCEntityHandlerResult CIRscManager::PUThandleStockRequest(OCEntityHandlerRequest* pReqEntity, OCRepPayload* data)
{
	ESLOG(LOG_DEBUG, "Enter.");

	OCEntityHandlerResult res = OC_EH_ERROR;
	res = handleStockRequest(pReqEntity, data);

	ESLOG(LOG_DEBUG, "Exit.");
	return res;
}

OCEntityHandlerResult CIRscManager::POSThandleStockRequest(OCEntityHandlerRequest* pReqEntity, OCRepPayload* data)
{
	ESLOG(LOG_DEBUG, "Enter.");

	OCEntityHandlerResult res = OC_EH_ERROR;
	res = handleStockRequest(pReqEntity, data);

	ESLOG(LOG_DEBUG, "Exit.");
	return res;
}

OCEntityHandlerResult CIRscManager::DELhandleStockRequest(OCEntityHandlerRequest* pReqEntity, OCRepPayload* data)
{
	ESLOG(LOG_DEBUG, "Enter.");

	if (data == NULL || pReqEntity == NULL)
	{
		ESLOG(LOG_ERROR, "Invalid Input argument.");
		return OC_EH_ERROR;
	}

	PrintRepPayloadValue(data->values);		// for Debugging.
											//OCRepPayloadGetPropInt(input, "power", &pow)

	ESLOG(LOG_ERROR, "Not Yet Support DELETE command.");

	ESLOG(LOG_DEBUG, "Exit.");
	return OC_EH_OK;
}

static OCResourceHandle CreateIoTivityResource(const char *uri, const char* rtName, const char* infName)
{
	OCResourceHandle handle = NULL;

	if (!uri)
	{
		ESLOG(LOG_ERROR, "Resource URI cannot be NULL");
		return NULL;
	}

	OCStackResult res = OCCreateResource(&handle,
		rtName,
		infName,
		uri,
		OCEntityHandlerCb,
		NULL,
		OC_DISCOVERABLE | OC_OBSERVABLE);

	ESLOG(LOG_INFO, "Created \"%s\" resource with result: %s", uri, IoTivityAPI::getResult(res));

	if (res != OC_STACK_OK)
	{
		handle = NULL;
	}

	return handle;
}

static void PrintRepPayloadValue(OCRepPayloadValue* values)
{
	if (values == NULL)
	{
		ESLOG(LOG_DEBUG, "Not Exist RepPayload-values");
		return ;
	}

	while (values)
	{
		ESLOG(LOG_DEBUG, "Key: %s", values->name);
		switch (values->type)
		{
		case OCREP_PROP_NULL:
			ESLOG(LOG_DEBUG, "Value: None");
			break;
		case OCREP_PROP_INT:
			ESLOG(LOG_DEBUG, "Value: %d", values->i);
			break;
		case OCREP_PROP_DOUBLE:
			ESLOG(LOG_DEBUG, "Value: %f", values->d);
			break;
		case OCREP_PROP_BOOL:
			ESLOG(LOG_DEBUG, "Value: %d", values->b);
			break;
		case OCREP_PROP_STRING:
			ESLOG(LOG_DEBUG, "Value: %s", values->str);
			break;
		case OCREP_PROP_BYTE_STRING:
			ESLOG(LOG_DEBUG, "Value: Byte String");
			break;
		case OCREP_PROP_OBJECT:
			ESLOG(LOG_DEBUG, "Value: Object");
			break;
		case OCREP_PROP_ARRAY:
			ESLOG(LOG_DEBUG, "Value: Array");
			break;
		default:
			break;
		}

		values = values->next;
	}
}


static OCEntityHandlerResult SendResponse(OCRequestHandle requestHandle,
										  OCResourceHandle resource,
										  OCEntityHandlerResult result,
										  void* payload)
{
	ESLOG(LOG_DEBUG, "Enter.");

	OCEntityHandlerResult ehResult = OC_EH_OK;
	OCEntityHandlerResponse response = { 0, 0, OC_EH_ERROR, 0, 0,{},{ 0 }, false };

	ESLOG(LOG_DEBUG, "Creating Response with Request Handle : %x,Resource Handle : %x", requestHandle, resource);

	response.numSendVendorSpecificHeaderOptions = 0;
	memset(response.sendVendorSpecificHeaderOptions,
		0, sizeof response.sendVendorSpecificHeaderOptions);
	memset(response.resourceUri, 0, sizeof(response.resourceUri));

	if (NULL != requestHandle && NULL != resource)
	{
		response.requestHandle = requestHandle;//ehRequest->requestHandle;
		response.resourceHandle = resource;//ehRequest->resource;
		response.persistentBufferFlag = 0;
		response.ehResult = result;
		response.payload = (OCPayload*)payload;

		if (payload != NULL)
			ESLOG(LOG_DEBUG, "Payload is Not NULL");
		else
			ESLOG(LOG_DEBUG, "No Payload");

		ESLOG(LOG_INFO, "\t\t\tRes. : %s ( %d )", (response.ehResult == OC_EH_OK ? "OK" : "NOT OK")
												, response.ehResult);

		OCStackResult ret = OCDoResponse(&response);		// Send Response.
		ESLOG(LOG_INFO, "\t\t\tMsg. Out? : (%s)", (ret == OC_STACK_OK) ? "SUCCESS" : "FAIL");
		if (ret != OC_STACK_OK)
		{
			ehResult = OC_EH_ERROR;
		}
	}
	else
	{
		ehResult = OC_EH_ERROR;
	}

	ESLOG(LOG_DEBUG, "Exit");
	return ehResult;
}

static OCEntityHandlerResult OCEntityHandlerCb(OCEntityHandlerFlag flag,
	OCEntityHandlerRequest *entityHandlerRequest, void* callback)
{
	ESLOG(LOG_DEBUG, "Enter");
	OCEntityHandlerResult ehResult = OC_EH_ERROR;

	// Validate pointer
	if (!entityHandlerRequest)
	{
		ESLOG(LOG_ERROR, "Invalid request pointer");
		return ehResult;
	}

	const char* uri = OCGetResourceUri(entityHandlerRequest->resource);

	// Observe Request Handling
	if (flag & OC_OBSERVE_FLAG)
	{
		if (OC_OBSERVE_REGISTER == entityHandlerRequest->obsInfo.action)
		{
			ESLOG(LOG_INFO, "Observe Requset on : %s ", uri);
			// 1. Check whether it's Observe request on the Collection Resource
			//if (NULL != strstr(uri, URI_DEVICE_COL))
			//{
			//	//2. Check whether the query carriese the if=oic.if.b
			//	if ((strstr(entityHandlerRequest->query, OIC_INTERFACE_BATCH) == NULL))
			//	{
			//		//3. If not batch then error
			//		ESLOG(LOG_ERROR, "Collection Resource Requires BATCH for Observing : %s", entityHandlerRequest->query);
			//		ehResult = OC_EH_BAD_REQ;
			//		goto RESPONSE_ERROR;
			//	}
			//	else
			//	{
			//		ESLOG(LOG_ERROR, "Receiving Observe Request Collection Resource");
			//	}
			//}
		}
		else if (OC_OBSERVE_DEREGISTER == entityHandlerRequest->obsInfo.action)
		{
			ESLOG(LOG_INFO, "CancelObserve Request on : %s", uri);
		}
	}

	// Get/Post Request Handling
	if (flag & OC_REQUEST_FLAG)
	{
		ESLOG(LOG_DEBUG, "Req. URI  : %s", uri);
		ESLOG(LOG_DEBUG, "Req. TYPE : %d", entityHandlerRequest->method);

		if (   (OC_REST_GET == entityHandlerRequest->method)
			|| (OC_REST_PUT == entityHandlerRequest->method)
			|| (OC_REST_POST == entityHandlerRequest->method))
		{
			ESLOG(LOG_DEBUG, "Request Handle : %x, Resource Handle : %x"
							 , entityHandlerRequest->requestHandle, entityHandlerRequest->resource);
			ESLOG(LOG_INFO, "Received method(0x%X) from client", entityHandlerRequest->method);

			if (pInstance == NULL)
			{
				ESLOG(LOG_INFO, "CIRscManager instance is NULL.");
				goto RESPONSE_ERROR;
			}

			ehResult = pInstance->CallHandler(entityHandlerRequest);

			//if (VerifyRequest(entityHandlerRequest, uri, (int)entityHandlerRequest->method) > 0)
			//{
			//	// [JAY] IoTivity Stack Destroy the payload after receving result from this function
			//	//       Therefore, we need to copy/clone the payload for the later use..
			//	OicResource* resource = CreateResourceInst(
			//		entityHandlerRequest->requestHandle,
			//		entityHandlerRequest->resource,
			//		entityHandlerRequest->query,
			//		entityHandlerRequest->payload);
			//	resource->SetDevAddr(resource, &(entityHandlerRequest->devAddr));
			//	//resource->SetUri(resource, uri);

			//	ESLOG(LOG_DEBUG, "About to Queue a received Request~!!!!");

			//	//gOicReqQueue.push(&gOicReqQueue, resource, entityHandlerRequest->method);
			//	gpOicReqQueue->push(gpOicReqQueue, resource, entityHandlerRequest->method);
			//	ehResult = OC_EH_SLOW;
			//}
			//else
			//{
			//	ESLOG(LOG_ERROR, "Invalid Query in the Request : %s",
			//		entityHandlerRequest->query);
			//}
		}
		else if (OC_REST_DELETE == entityHandlerRequest->method)
		{
			ESLOG(LOG_ERROR, "Delete Req. Handling is not supported Yet");
		}
		else
		{
			ESLOG(LOG_DEBUG, "Received unsupported method from client");
		}

	}

RESPONSE_ERROR:
	if (ehResult != OC_EH_SLOW)
	{
		// [JAY] If the result is OC_EH_ERROR, the stack will remove the
		//       received request in the stack.
		//       If the reusult is OC_EH_SLOW, then the request will be
		//       stored in the stack till the response goes out
		ehResult = SendResponse(entityHandlerRequest->requestHandle,
								entityHandlerRequest->resource,
								ehResult, NULL);
	}

	ESLOG(LOG_DEBUG, "Exit");
	return ehResult;

/*
	ESLOG(LOG_INFO, "Inside entity handler - flags: 0x%x", flag);

	OCRepPayload* payload = NULL;
	OCEntityHandlerResult ehResult = OC_EH_OK;
	OCEntityHandlerResponse response = { 0, 0, OC_EH_ERROR, 0, 0,{},{ 0 }, false };

	// Validate pointer
	if (!pReq)
	{
		ESLOG(LOG_ERROR, "Invalid request pointer");
		return OC_EH_ERROR;
	}

	// Initialize certain response fields
	response.numSendVendorSpecificHeaderOptions = 0;
	memset(response.sendVendorSpecificHeaderOptions,
		0, sizeof response.sendVendorSpecificHeaderOptions);
	memset(response.resourceUri, 0, sizeof response.resourceUri);

	if (flag & OC_REQUEST_FLAG)
	{
		ESLOG(LOG_INFO, "Flag includes OC_REQUEST_FLAG. pInstance = 0x%X", pInstance);

		if (pInstance != NULL)
		{
			ESLOG(LOG_INFO, "Received method(0x%X) from client", pReq->method);
			ehResult = pInstance->CallHandler(pReq->resource, pReq->method, 
				                               (OCRepPayload*)pReq->payload);
		}

		// If the result isn't an error or forbidden, send response
		if (!((ehResult == OC_EH_ERROR) || (ehResult == OC_EH_FORBIDDEN))  )
		{
			// Format the response.  Note this requires some info about the request
			response.requestHandle = pReq->requestHandle;
			response.resourceHandle = pReq->resource;
			response.ehResult = ehResult;
			response.payload = reinterpret_cast<OCPayload*>(payload);
			// Indicate that response is NOT in a persistent buffer
			response.persistentBufferFlag = 0;

			// Handle vendor specific options
			if (pReq->rcvdVendorSpecificHeaderOptions &&
				pReq->numRcvdVendorSpecificHeaderOptions)
			{
				ESLOG(LOG_INFO, "Received vendor specific options");
				uint8_t i = 0;
				OCHeaderOption * rcvdOptions =
					pReq->rcvdVendorSpecificHeaderOptions;
				for (i = 0; i < pReq->numRcvdVendorSpecificHeaderOptions; i++)
				{
					if (((OCHeaderOption)rcvdOptions[i]).protocolID == OC_COAP_ID)
					{
						ESLOG(LOG_INFO, "Received option with OC_COAP_ID and ID %u with",
							((OCHeaderOption)rcvdOptions[i]).optionID);

						//OIC_LOG_BUFFER(INFO, TAG, ((OCHeaderOption)rcvdOptions[i]).optionData,
						//	MAX_HEADER_OPTION_DATA_LENGTH);
					}
				}

				OCHeaderOption* sendOptions = response.sendVendorSpecificHeaderOptions;
				size_t numOptions = response.numSendVendorSpecificHeaderOptions;
				// Check if the option header has already existed before adding it in.
				uint8_t optionData[MAX_HEADER_OPTION_DATA_LENGTH];
				size_t optionDataSize = sizeof(optionData);
				uint16_t actualDataSize = 0;
				OCGetHeaderOption(response.sendVendorSpecificHeaderOptions,
					response.numSendVendorSpecificHeaderOptions,
					2248,
					optionData,
					optionDataSize,
					&actualDataSize);
				if (actualDataSize == 0)
				{
					uint8_t option2[] = { 21,22,23,24,25,26,27,28,29,30 };
					uint16_t optionID2 = 2248;
					size_t optionDataSize2 = sizeof(option2);
					OCSetHeaderOption(sendOptions,
						&numOptions,
						optionID2,
						option2,
						optionDataSize2);
				}

				OCGetHeaderOption(response.sendVendorSpecificHeaderOptions,
					response.numSendVendorSpecificHeaderOptions,
					2600,
					optionData,
					optionDataSize,
					&actualDataSize);
				if (actualDataSize == 0)
				{
					uint8_t option3[] = { 31,32,33,34,35,36,37,38,39,40 };
					uint16_t optionID3 = 2600;
					size_t optionDataSize3 = sizeof(option3);
					OCSetHeaderOption(sendOptions,
						&numOptions,
						optionID3,
						option3,
						optionDataSize3);
				}
				response.numSendVendorSpecificHeaderOptions = 2;
			}

			// Send the response
			if (OCDoResponse(&response) != OC_STACK_OK)
			{
				ESLOG(LOG_ERROR, "Error sending response");
				ehResult = OC_EH_ERROR;
			}
		}
	}

	if (flag & OC_OBSERVE_FLAG)
	{
		ESLOG(LOG_INFO, "Flag includes OC_OBSERVE_FLAG");

		if (OC_OBSERVE_REGISTER == pReq->obsInfo.action)
		{
			ESLOG(LOG_INFO, "Received OC_OBSERVE_REGISTER from client");
			//ProcessObserveRegister(pReq);
		}
		else if (OC_OBSERVE_DEREGISTER == pReq->obsInfo.action)
		{
			ESLOG(LOG_INFO, "Received OC_OBSERVE_DEREGISTER from client");
			//ProcessObserveDeregister(pReq);
		}
	}

	OCPayloadDestroy(response.payload);
	return ehResult;
*/
}






/***************************************************************************************
 *
 * ChandlerCol Class Definition.
 *
 ***************************************************************************************/
void CIRscManager::ChandlerCol::initial(const char* URI, handlerFuncType getHandler,
	handlerFuncType putHandler, handlerFuncType postHandler, handlerFuncType deleteHandler)
{
	uri = ESstrdup(URI);
	GEThandler = getHandler;
	PUThandler = putHandler;
	POSThandler = postHandler;
	DELETEhandler = deleteHandler;
}






//OicResource* CreateResourceInstImpl(void* requesthd, void* resourcehd, void* query, void* repPayload)
//{
//	OicResource* res = (OicResource*)DAMalloc(sizeof(OicResource));
//	if (NULL == res)
//	{
//		DA_LOG(DA_ERROR, TAG, DA_MEMORY_ERROR);
//		return NULL;
//	}
//
//	res->error = 0;                               // OC_EH_OK;
//
//	res->SetDevAddr = &SetDevAddr;
//	res->GetDevAddr = &GetDevAddr;
//
//	res->SetUri = &SetUri;
//	res->SetError = &SetError;
//
//	res->SetCommandId = &SetCommandId;
//
//	res->AddChild = &AddChild;
//	res->GetChildren = &GetChildren;
//
//	res->GetUri = &GetUri;
//	res->GetResType = &GetResType;
//	res->GetNumOfResTypes = &GetNumOfResTypes;
//	res->GetInfType = &GetInfType;
//	res->GetNumOfInfTypes = &GetNumOfInfTypes;
//	res->SetRepresentation = &SetRepresentation;
//	res->GetRepresentation = &GetRepresentation;
//
//	res->GetQuery = &GetQuery;
//	res->GetRepPayload = &GetRepPayload;
//	res->CreatePayload = &CreatePayload;
//	res->IsSupportingInterfaceType = &IsSupportingInterfaceType;
//	res->IsSupportingResourceType = &IsSupportingResourceType;
//
//	res->next = NULL;
//	res->Next = &Next;
//
//	res->resourceHandle = resourcehd;
//	res->requestHandle = requesthd;
//
//	res->uri = NULL;
//	const char * uri = OCGetResourceUri(resourcehd);
//	if (uri != NULL && strlen(uri) > 0)
//	{
//		res->uri = (char*)DAMalloc(sizeof(char) * strlen(uri) + 1);
//		memset(res->uri, 0, strlen(uri) + 1);
//		DAStrncpy(res->uri, uri, strlen(uri));
//	}
//
//	res->resType = NULL;
//	res->interfaceType = NULL;
//	res->reqType = 0;
//	res->cmdId = NULL;
//	res->rep = NULL;
//	res->query = NULL;
//	res->size = 1;
//
//	if (NULL != query)
//	{
//		DuplicateString((char*)query, &(res->query));
//	}
//
//	if (NULL != repPayload)
//	{
//		res->rep = CreateRepresentationInst(repPayload);
//	}
//	// else
//	// {
//	//     DA_LOG_D(DA_DEBUG,TAG, "Representation not created!!");
//	// }
//
//	return res;
//}
//
//
//OicResource* CreateResourceInst(void* requesthd, void* resourcehd, void* query, void* repPayload)
//{
//	OicResource* res = CreateResourceInstImpl(requesthd,
//		resourcehd,
//		query,
//		repPayload);
//
//	if (NULL != repPayload)
//	{
//		DA_LOG_D(DA_DEBUG, TAG, "Representation Inserted to Clone~!!!");
//		OCRepPayload* pl = ((OCRepPayload*)repPayload)->next;
//
//		OicResource* pTempRes = NULL;
//		while (pl)
//		{
//			if (NULL != pl->uri)
//			{
//				pTempRes = CreateResourceInstImpl(requesthd,
//					resourcehd,
//					NULL,
//					pl);
//
//				DA_LOG_D(DA_DEBUG, TAG, "Child Resource URI : %s", pl->uri);
//				pTempRes->SetUri(pTempRes, pl->uri);
//				res->AddChild(res, pTempRes);
//				//res->size++;
//			}
//			else
//			{
//				DA_LOG_D(DA_DEBUG, TAG,
//					"No Uri in this payload for child, this payload was ignored.");
//			}
//
//			pl = pl->next;
//		}
//		DA_LOG_D(DA_DEBUG, TAG, "\t Child Count : %d", res->size);
//	}
//	else
//	{
//		DA_LOG_D(DA_DEBUG, TAG, "No Representation to Clone : %x", repPayload);
//	}
//
//	return res;
//}


//void /*OCRepPayload*/* GetRepPayload(struct OicResource* res)
//{
//	if (res->rep != NULL)
//	{
//		// Create Payload for the mother resource
//		// It applies not only single resource but also the collection resource..
//		OicResource* pTemp;
//		OCRepPayload* repPayload;
//
//		if (NULL != res->GetChildren(res) && (NULL != strstr(res->uri, URI_DEVICE_COL)) && (strstr(res->query, OIC_INTERFACE_BATCH) != NULL))
//		{
//			pTemp = res->next;
//			repPayload = pTemp->CreatePayload(pTemp, pTemp->query);
//			pTemp = pTemp->next;
//		}
//		else
//		{
//			repPayload = res->CreatePayload(res, res->query);
//			// Create payload for the children resource(s)
//			pTemp = res->GetChildren(res);
//		}
//
//		while (NULL != pTemp)
//		{
//			if (NULL != pTemp->rep)
//			{
//				OCRepPayloadAppend(repPayload,
//					pTemp->CreatePayload(pTemp, pTemp->query));
//			}
//			pTemp = pTemp->next;
//		}
//
//		return repPayload;
//	}
//	else
//	{
//		DA_LOG_D(DA_ERROR, TAG, "ROOT(Parent) Paylaod is NULL.");
//		return NULL;
//	}
//}



//void /*OCRepPayload*/* CreatePayload(struct OicResource* res, char* query)
//{
//	// [JAY] To provide identical pattern for handling the result
//	//       allocating new memory for the payload to delete later
//	OCRepPayload* payload = OCRepPayloadClone((OCRepPayload*)(res->rep->payload));
//	DA_LOG_D(DA_DEBUG, TAG, "Query : %s", query);
//
//	if (query == NULL
//		|| (query != NULL && strlen(query) < 1)
//		|| strstr(query, OIC_INTERFACE_ACTUATOR) != NULL
//		|| strstr(query, OIC_INTERFACE_SENSOR) != NULL)
//	{
//		DA_LOG_D(DA_DEBUG, TAG, "Including properties & its values only");
//		// Do nothing..
//	}
//	else if (strstr(query, OIC_INTERFACE_BASELINE))
//	{
//		DA_LOG_D(DA_DEBUG, TAG, "Including all the properties & its values");
//
//		uint8_t index = 0;
//		uint8_t numberOfInterfaces = 0;
//		uint8_t numberOfResourceType = 0;
//
//		OCGetNumberOfResourceInterfaces((OCResourceHandle)res->resourceHandle, &numberOfInterfaces);
//
//		DA_LOG_D(DA_DEBUG, TAG, "@@  IF # : %d ", numberOfInterfaces);
//		//2.a Find interface type(s) & insert it/them into payload
//		for (index = 0; index < numberOfInterfaces; index++)
//		{
//			const char* interface = OCGetResourceInterfaceName((OCResourceHandle)res->resourceHandle, index);
//			DA_LOG_D(DA_DEBUG, TAG, "=====>  IF : %s ", interface);
//			OCRepPayloadAddInterface(payload, interface);
//		}
//
//		//3.a Find resource type & insert it into payload
//		OCGetNumberOfResourceTypes((OCResourceHandle)res->resourceHandle, &numberOfResourceType);
//		DA_LOG_D(DA_DEBUG, TAG, "@@  RT # : %d ", numberOfResourceType);
//		for (index = 0; index < numberOfResourceType; index++)
//		{
//			const char* rt = OCGetResourceTypeName((OCResourceHandle)res->resourceHandle, index);
//			DA_LOG_D(DA_DEBUG, TAG, "=====>  RT : %s ", rt);
//			OCRepPayloadAddResourceType(payload, rt);
//		}
//	}
//	else if (strstr(query, OC_RSRVD_INTERFACE_BATCH))
//	{
//		DA_LOG_D(DA_DEBUG, TAG, "Batch only supported by Collection Resource");
//	}
//	else if (strstr(query, OC_RSRVD_INTERFACE_LL))
//	{
//		DA_LOG_D(DA_DEBUG, TAG, "Link-List only supported by Collection Resource");
//	}
//	else
//	{
//		DA_LOG_D(DA_DEBUG, TAG, "Not supporting Interface type : %s", query);
//	}
//
//	return payload;
//}

