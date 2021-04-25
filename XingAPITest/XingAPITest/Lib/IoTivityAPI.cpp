#include "Config.h"
#include <Windows.h>
#include <pthread.h>

#define LOGTAG	"IoTivityAPI"
#include <dlogger.h>

#include "IoTivityAPI.h"
#include "EScommon.h"

// iotivity headers.
#include "iotivity_config.h"
#include "ocpayload.h"

// Instance variable을 직접 사용하는 곳은 GetInstance, Initialize와 ~IoTivityAPI 함수에서만 허용한다.
IoTivityAPI* IoTivityAPI::Instance = NULL;
vCIoTReqParsor* IoTivityAPI::IotReqParsorIns = NULL;
bool IoTivityAPI::bInit = false;
void* IoTivityAPI::threadHandle = NULL;
static pthread_mutex_t mutex_instance = PTHREAD_MUTEX_INITIALIZER;

typedef int (IoTivityAPI::*pIOTFuncType)(void);


static int InitDiscovery(OCQualityOfService qos);
OCStackApplicationResult discoveryReqCB(void* ctx, OCDoHandle, OCClientResponse * clientResponse);

IoTivityAPI* IoTivityAPI::GetInstance(void)
{	
	IoTivityAPI* instance = NULL;
	
	pthread_mutex_lock(&mutex_instance);
	if (Instance == NULL)
	{
		Instance = new IoTivityAPI();
	}
	instance = Instance;
	pthread_mutex_unlock(&mutex_instance);

	return instance;
}

vCIoTReqParsor* IoTivityAPI::GetInstanceReqParsor(void)
{
	vCIoTReqParsor* instance = NULL;

	pthread_mutex_lock(&mutex_instance);
	if (Instance == NULL)
	{
		instance = NULL;
	}
	else
	{
		instance = Instance->IotReqParsorIns;
	}
	pthread_mutex_unlock(&mutex_instance);

	return instance;
}

IoTivityAPI::IoTivityAPI(void)
{
	ESLOG(LOG_DEBUG, "Create Instance.");
}

int IoTivityAPI::initialize(OCMode mode, vCIoTReqParsor* pIotReqParsor)
{
	int result = -1;
	pthread_attr_t threadAttr = NULL;

	if (pIotReqParsor == NULL)
	{
		ESLOG(LOG_ERROR, "pIotReqParsor is NULL.");
		return result;
	}

	pthread_mutex_lock(&mutex_instance);
	if (Instance == NULL)
	{
		ESLOG(LOG_ERROR, "Instance is NULL.");
		goto GOTO_EXIT;
	}

	if (bInit == true)
	{
		ESLOG(LOG_DEBUG, "Already, Initialize is done.");
		result = 0;
		goto GOTO_EXIT;
	}

	if (OCInit1(mode, OC_DEFAULT_FLAGS, OC_DEFAULT_FLAGS) != OC_STACK_OK)
	{
		ESLOG(LOG_ERROR, "OCStack init error");
		goto GOTO_EXIT;
	}

	// Stock-Task thread create
	if (pthread_attr_init(&threadAttr) == -1) {
		ESLOG(LOG_ERROR, "error in pthread_attr_init");
		goto GOTO_EXIT;
	}

	if (pthread_attr_setstacksize(&threadAttr, 8192) == -1) {
		ESLOG(LOG_ERROR, "error in pthread_attr_setstacksize");
		goto GOTO_EXIT;
	}

	if ((threadHandle = ESmalloc(sizeof(pthread_t))) == NULL)
	{
		ESLOG(LOG_ERROR, "pthread_t malloc is failed.");
		goto GOTO_EXIT;
	}

	IotReqParsorIns = pIotReqParsor;
	pIOTFuncType pIOTFunc = &IoTivityAPI::MainThread;
	pThreadFuncType pFunc = *(pThreadFuncType*)&pIOTFunc;
	if (pthread_create(PTHREAD_HANDLE(threadHandle), &threadAttr, pFunc, (void*)this) == -1)
	{
		ESLOG(LOG_ERROR, "error in pthread_create for SockMainThread");
		ESfree(threadHandle);
		threadHandle = NULL;
		goto GOTO_EXIT;
	}
	ESLOG(LOG_DEBUG, "this = 0x%X", this);

	bInit = true;
	result = 1;

GOTO_EXIT:
	if(threadAttr != NULL)
		pthread_attr_destroy(&threadAttr);

	pthread_mutex_unlock(&mutex_instance);

	return result;
}

void IoTivityAPI::RelInstance(void)
{
	pthread_mutex_lock(&mutex_instance);
	if (Instance != NULL)
	{
		if (bInit == true)
		{
			bInit = false;

			if (threadHandle)
			{
				pthread_join(*PTHREAD_HANDLE(threadHandle), NULL);
				ESfree(threadHandle);
				threadHandle = NULL;
			}

			if (OCStop() != OC_STACK_OK)
			{
				ESLOG(LOG_ERROR, "OCStack stop error");
			}
		}
		Instance = NULL;
		IotReqParsorIns = NULL;
	}
	pthread_mutex_unlock(&mutex_instance);
}

IoTivityAPI::~IoTivityAPI(void)
{
	RelInstance();
}

#include "CIRscManager.h"
CIRscManager* ResourceManager = NULL;
/**************************************************
 *
 *	Main Thread of IoTivity Library.
 *
**************************************************/
int IoTivityAPI::MainThread(void)
{
	ESLOG(LOG_DEBUG, "Enter.");

	bool isCreated = false;
	if ((ResourceManager = new CIRscManager(&isCreated)) == NULL || isCreated == false)
	{
		ESLOG(LOG_DEBUG, "Creating Resource Manager is failed.");
		return 1;
	}

	if (InitDiscovery(OC_MEDIUM_QOS) != OC_STACK_OK)
	{
		ESLOG(LOG_DEBUG, "Discovery is failed.");
		return 1;
	}

	while (bInit)
	{
		printf("#");
		if (OCProcess() != OC_STACK_OK)
		{
			ESLOG(LOG_ERROR, "OCStack process error");
			return 0;
		}
		sleep(1);
	}

	if (ResourceManager)
	{
		delete ResourceManager;
		ResourceManager = NULL;
	}

	ESLOG(LOG_DEBUG, "Exit.");
	return 1;
}



const char *IoTivityAPI::getResult(OCStackResult result)
{
	switch (result)
	{
	case OC_STACK_OK:
		return "OC_STACK_OK";
	case OC_STACK_RESOURCE_CREATED:
		return "OC_STACK_RESOURCE_CREATED";
	case OC_STACK_RESOURCE_DELETED:
		return "OC_STACK_RESOURCE_DELETED";
	case OC_STACK_RESOURCE_CHANGED:
		return "OC_STACK_RESOURCE_CHANGED";
	case OC_STACK_INVALID_URI:
		return "OC_STACK_INVALID_URI";
	case OC_STACK_INVALID_QUERY:
		return "OC_STACK_INVALID_QUERY";
	case OC_STACK_INVALID_IP:
		return "OC_STACK_INVALID_IP";
	case OC_STACK_INVALID_PORT:
		return "OC_STACK_INVALID_PORT";
	case OC_STACK_INVALID_CALLBACK:
		return "OC_STACK_INVALID_CALLBACK";
	case OC_STACK_INVALID_METHOD:
		return "OC_STACK_INVALID_METHOD";
	case OC_STACK_NO_MEMORY:
		return "OC_STACK_NO_MEMORY";
	case OC_STACK_COMM_ERROR:
		return "OC_STACK_COMM_ERROR";
	case OC_STACK_INVALID_PARAM:
		return "OC_STACK_INVALID_PARAM";
	case OC_STACK_NOTIMPL:
		return "OC_STACK_NOTIMPL";
	case OC_STACK_NO_RESOURCE:
		return "OC_STACK_NO_RESOURCE";
	case OC_STACK_RESOURCE_ERROR:
		return "OC_STACK_RESOURCE_ERROR";
	case OC_STACK_SLOW_RESOURCE:
		return "OC_STACK_SLOW_RESOURCE";
	case OC_STACK_NO_OBSERVERS:
		return "OC_STACK_NO_OBSERVERS";
	case OC_STACK_UNAUTHORIZED_REQ:
		return "OC_STACK_UNAUTHORIZED_REQ";
#ifdef WITH_PRESENCE
	case OC_STACK_PRESENCE_STOPPED:
		return "OC_STACK_PRESENCE_STOPPED";
	case OC_STACK_PRESENCE_TIMEOUT:
		return "OC_STACK_PRESENCE_TIMEOUT";
#endif
	case OC_STACK_ERROR:
		return "OC_STACK_ERROR";
	default:
		return "UNKNOWN";
	}
}






#define DEFAULT_CONTEXT_VALUE 0x99

static char discoveryAddr[100];
static const char *DEVICE_DISCOVERY_QUERY = "%s/oic/d";
static const char *PLATFORM_DISCOVERY_QUERY = "%s/oic/p";
static const char *RESOURCE_DISCOVERY_QUERY = "%s/oic/res";
static OCConnectivityType ConnType = CT_ADAPTER_IP;
static OCDevAddr serverAddr;
static const char *coapServerResource = "/a/light";

static char* getConnectivityType(OCConnectivityType connType);

static int InitDiscovery(OCQualityOfService qos)
{
	OCStackResult ret;
	OCCallbackData cbData;
	char szQueryUri[MAX_QUERY_LENGTH] = { 0 };

	discoveryAddr[0] = '\0';

	snprintf(szQueryUri, sizeof(szQueryUri) - 1, RESOURCE_DISCOVERY_QUERY, discoveryAddr);
	ESLOG(LOG_DEBUG, "query URI=%s", szQueryUri);

	cbData.cb = discoveryReqCB;
	cbData.context = (void*)DEFAULT_CONTEXT_VALUE;
	cbData.cd = NULL;

	ret = OCDoResource(NULL, OC_REST_DISCOVER, szQueryUri, NULL, 0, CT_DEFAULT,
		qos, &cbData, NULL, 0);
	if (ret != OC_STACK_OK)
	{
		ESLOG(LOG_DEBUG, "OCStack resource error");
	}

	return ret;
}


OCStackApplicationResult discoveryReqCB(void* ctx, OCDoHandle /*handle*/,
	OCClientResponse * clientResponse)
{
	if (ctx == (void*)DEFAULT_CONTEXT_VALUE)
	{
		ESLOG(LOG_INFO, "Callback Context for DISCOVER query recvd successfully");
	}

	if (clientResponse)
	{
		ESLOG(LOG_INFO, "StackResult: %s", IoTivityAPI::getResult(clientResponse->result));

		const char* connectionType = getConnectivityType(clientResponse->connType);
		ESLOG(LOG_INFO, "Discovered on %s", connectionType);
		ESLOG(LOG_INFO,
			"Device =============> Discovered @ %s:%d",
			clientResponse->devAddr.addr,
			clientResponse->devAddr.port);

		ConnType = clientResponse->connType;
		serverAddr = clientResponse->devAddr;

		OCDiscoveryPayload *payload = (OCDiscoveryPayload*)clientResponse->payload;
		if (!payload)
		{
			ESLOG(LOG_DEBUG, "payload is NULL.");
			return OC_STACK_DELETE_TRANSACTION;
		}

		OCResourcePayload *resource = (OCResourcePayload*)payload->resources;

		int found = 0;
		int rscCnt = 0;
		while (resource)
		{
			ESLOG(LOG_DEBUG, "[ RSC: %d th ]", rscCnt++);
			ESLOG(LOG_INFO, "URI = %s", resource->uri);
			ESLOG(LOG_INFO, "Port = %d", resource->port);
			ESLOG(LOG_INFO, "Secure = %d", resource->secure);

			if (resource->uri && strcmp(resource->uri, coapServerResource) == 0)
			{
				found = 1;
				break;
			}
			resource = resource->next;
		}

		if (!found)
		{
			ESLOG(LOG_INFO, "No %s in payload", coapServerResource);
			return OC_STACK_KEEP_TRANSACTION;
		}
		else
		{
			ESLOG(LOG_INFO, ">>>>> Found Resource(%s) <<<<<<", coapServerResource);
		}

	}
	else
	{
		ESLOG(LOG_INFO, "discoveryReqCB received Null clientResponse");
	}
	return OC_STACK_KEEP_TRANSACTION;
}




static char* getConnectivityType(OCConnectivityType connType)
{
	switch (connType & CT_MASK_ADAPTER)
	{
	case CT_ADAPTER_IP:
		return "IP";

	case CT_IP_USE_V4:
		return "IPv4";

	case CT_IP_USE_V6:
		return "IPv6";

	case CT_ADAPTER_GATT_BTLE:
		return "GATT";

	case CT_ADAPTER_RFCOMM_BTEDR:
		return "RFCOMM";

	default:
		ESLOG(LOG_DEBUG, "connType=%d", connType);
		return "Incorrect connectivity";
	}
}
