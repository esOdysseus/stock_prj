#ifndef  IOTIVITY_API_H_EUNSEOK_KIM
#define IOTIVITY_API_H_EUNSEOK_KIM

#include "ocstack.h"
#include "vCIoTREQParsor.h"

// Singleton Class.
class IoTivityAPI {
private :
	// member variables
	static IoTivityAPI* Instance;

	static vCIoTReqParsor* IotReqParsorIns;

	static bool bInit;

	static void* threadHandle;

	// member functions
	IoTivityAPI(void);

	int MainThread(void);

	void RelInstance(void);

public :
	~IoTivityAPI(void);

	static IoTivityAPI* GetInstance(void);

	static vCIoTReqParsor* GetInstanceReqParsor(void);

	int initialize(OCMode mode, vCIoTReqParsor* pIotReqParsor);

	static const char *getResult(OCStackResult result);
};


#endif // IOTIVITY_API_H_EUNSEOK_KIM