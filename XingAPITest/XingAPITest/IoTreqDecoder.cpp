#include "IoTreqDecoder.h"

#define LOGTAG	"iotReqDecoder"
#include "dlogger.h"

#include <Windows.h>
#include "EScommon.h"





iotReqDecoder::iotReqDecoder(OCPayload* rawData)
{
	minorCmdEA = 0;
	minorCmd = NULL;
	parsingSuccess = false;

	if ( (parsingSuccess=parsing((OCRepPayload*)rawData)) == false)
	{
		ESLOG(LOG_ERROR, "Request Parsing is failed.");
	}
}

iotReqDecoder::~iotReqDecoder(void)
{
	ESLOG(LOG_DEBUG, "Enter.");

	if (majorCmd.Name)
	{
		ESfree(majorCmd.Name);
		majorCmd.Name = NULL;
	}

	majorCmd.Params.clear();

	if (minorCmdEA > 0)
	{
		if (minorCmd == NULL)
		{
			ESLOG(LOG_ERROR, "minorCmd is Must not NULL. but it's NULL.");
			return;
		}

		for (int i = 0; i < minorCmdEA; i++)
		{
			if (minorCmd[i].Name)
			{
				ESfree(minorCmd[i].Name);
				minorCmd[i].Name = NULL;
			}

			minorCmd[i].Params.clear();
		}

		minorCmdEA = 0;
	}

	ESLOG(LOG_DEBUG, "Exit.");
}

bool iotReqDecoder::GetResultParsing(void)
{
	return parsingSuccess;
}

char* iotReqDecoder::GetMajorCmd(void)
{
	if (majorCmd.Name == NULL)
	{
		ESLOG(LOG_INFO, "MajorCmd name is NULL.");
		return NULL;
	}

	return ESstrdup(majorCmd.Name);
}

iotReqDecoder::ParamMap* iotReqDecoder::GetMajorParams(void)
{
	return &majorCmd.Params;
}

void* iotReqDecoder::GetMajorParamVal(const char* key, PARAMVAL_DATATYPE* dataType)
{
	ESLOG(LOG_DEBUG, "Enter.");

	ParamMap::iterator itor;
	char* strVal = NULL;
	int intVal = 0;
	void* res = NULL;

	if (key == NULL || dataType == NULL)
	{
		ESLOG(LOG_ERROR, "invalid input.(key=0x%X, dataType=0x%X)", key, dataType);
		return NULL;
	}

	if ((itor = majorCmd.Params.find(key)) == majorCmd.Params.end())
	{
		ESLOG(LOG_INFO, "key:value set is not exist about key(\"%s\")", key);
		return NULL;
	}

	if ( (*dataType = itor->second.get(strVal, &intVal)) == VALTYPE_NULL )
	{
		ESLOG(LOG_INFO, "value is invalid.(key=%s, datatype=%s)", key, *dataType);
		return NULL;
	}

	switch (*dataType)
	{
	case VALTYPE_STRING:
		res = (void*)strVal;
		break;
	case VALTYPE_INTEGER:
		res = (void*)intVal;
		break;
	default :
		ESLOG(LOG_ERROR, "Not-Support data-type.");
		break;
	}

	ESLOG(LOG_DEBUG, "Exit.");
	return res;
}

int iotReqDecoder::GetMinorCmdEA(void)
{
	return minorCmdEA;
}

char* iotReqDecoder::GetMinorCmd(int index)
{
	if (minorCmd == NULL || minorCmd[index].Name == NULL)
	{
		ESLOG(LOG_INFO, "MinorCmd or name is NULL. (minorCmd=0x%X, name=%s)", minorCmd, minorCmd[index].Name);
		return NULL;
	}

	return ESstrdup(minorCmd[index].Name);
}

iotReqDecoder::ParamMap* iotReqDecoder::GetMinorParams(int index)
{
	if (minorCmd == NULL || minorCmd[index].Name == NULL)
	{
		ESLOG(LOG_INFO, "MinorCmd or name is NULL. (minorCmd=0x%X, name=%s)", minorCmd, minorCmd[index].Name);
		return NULL;
	}

	return &minorCmd[index].Params;
}

void* iotReqDecoder::GetMinorParamVal(int index, const char* key, PARAMVAL_DATATYPE* dataType)
{
	ESLOG(LOG_DEBUG, "Enter.");

	ParamMap::iterator itor;
	char* strVal = NULL;
	int intVal = 0;
	void* res = NULL;

	if (key == NULL || dataType == NULL || minorCmd == NULL || minorCmdEA == 0 || index < 0 || index >= minorCmdEA)
	{
		ESLOG(LOG_ERROR, "invalid input value.(key=%s, dataType=0x%X, minorCmd=0x%X, minorCmdEA=%d, index=%d)", key, dataType, minorCmd, minorCmdEA, index);
		return NULL;
	}

	if ((itor = minorCmd[index].Params.find(key)) == minorCmd[index].Params.end())
	{
		ESLOG(LOG_INFO, "key:value set is not exist about key(\"%s\")", key);
		return NULL;
	}

	if ((*dataType = itor->second.get(strVal, &intVal)) == VALTYPE_NULL)
	{
		ESLOG(LOG_INFO, "value is invalid.(key=%s, datatype=%s)", key, *dataType);
		return NULL;
	}

	switch (*dataType)
	{
	case VALTYPE_STRING:
		res = (void*)strVal;
		break;
	case VALTYPE_INTEGER:
		res = (void*)intVal;
		break;
	default:
		ESLOG(LOG_ERROR, "Not-Support data-type.");
		break;
	}

	ESLOG(LOG_DEBUG, "Exit.");
	return res;
}


/****************************************************
 * Internal Private Class Function.
 */

#include "ocpayload.h"

#define KEY_COMMAND		(const char*)"cmd"
#define KEY_PARAMETER	(const char*)"param"
#define KEY_MINORCMDS	(const char*)"minor"
#define KEY_MINORCMDEA	(const char*)"minorea"

OCRepPayload** GetObjectArrayFromPayload(const OCRepPayload* rawData, const char* key, int * resEA);

/**
 * IoT request Parsing Function.
 * Support Message format
 * {
 *    cmd:"majorCmd",param:[]
 *    ,minorea:2,minor:[{cmd:"minorCmd01",param:[]},{cmd:"minorCmd02",param:[]}]
 * }
 */
bool iotReqDecoder::parsing(const OCRepPayload* rawData)
{
	int paramEA = 0;
	int minorEA = 0;
	bool res = false;
	OCRepPayload** paramData = NULL;
	OCRepPayload** minorData = NULL;

	if (rawData == NULL)
	{
		ESLOG(LOG_ERROR, "rawData is not exist.(NULL)");
		return res;
	}

	//bool OCRepPayloadGetPropString(const OCRepPayload* payload, const char* name, char** value);
	// Major Command parsing.
	OCRepPayloadGetPropString(rawData, KEY_COMMAND, &majorCmd.Name);
	if ((paramData = GetObjectArrayFromPayload(rawData, KEY_PARAMETER, &paramEA)) == NULL || paramEA != 1)
	{
		ESLOG(LOG_ERROR, "paramData is NULL.(paramEA=%d)", paramEA);
		return res;
	}

	OCRepPayloadValue* val = paramData[0]->values;
	while (val)
	{
		ESLOG(LOG_DEBUG, "insert Values ...");
		majorCmd.Params[val->name].set(val);
		val = val->next;
	}
	OCPayloadDestroy((OCPayload*)paramData[0]);

	
	// Minor Commands parsing.
	int64_t temp = 0;
	OCRepPayloadGetPropInt(rawData, KEY_MINORCMDEA, &temp);
	ESLOG(LOG_DEBUG, "expected minorCmdEA=%d", temp);
	if (temp > 0)
	{
		minorCmdEA = (int)temp;
		if ((minorData = GetObjectArrayFromPayload(rawData, KEY_MINORCMDS, &minorEA)) == NULL || minorEA != minorCmdEA)
		{
			ESLOG(LOG_ERROR, "minorData is NULL.(minorEA=%d)", minorEA);
			return res;
		}

		if ((minorCmd = (sCmd*)malloc(sizeof(sCmd)*minorEA)) == NULL)
		{
			ESLOG(LOG_ERROR, "minorCmd allocation is failed.");
			return res;
		}

		for (int i = 0; i < minorEA; i++)
		{
			paramEA = 0;
			paramData = NULL;
			OCRepPayloadGetPropString(minorData[i], KEY_COMMAND, &minorCmd[i].Name);
			if ((paramData = GetObjectArrayFromPayload(minorData[i], KEY_PARAMETER, &paramEA)) == NULL || paramEA != 1)
			{
				ESLOG(LOG_ERROR, "paramData is NULL.(paramEA=%d)", paramEA);
				return res;
			}

			OCRepPayloadValue* val = paramData[0]->values;
			while (val)
			{
				ESLOG(LOG_DEBUG, "insert Values ...");
				minorCmd[i].Params[val->name].set(val);
				val = val->next;
			}

			OCPayloadDestroy((OCPayload*)paramData[0]);
			OCPayloadDestroy((OCPayload*)minorData[i]);
		}
	}

	res = true;
	return res;
}


static OCRepPayload** GetObjectArrayFromPayload(const OCRepPayload* rawData, const char* key, int * resEA)
{
	ESLOG(LOG_DEBUG, "Enter.");

	OCRepPayload** payloadValues = NULL;
	OCRepPayloadValue* val = NULL;

	if (rawData == NULL || key == NULL || resEA == NULL)
	{
		ESLOG(LOG_DEBUG, "invalid input Data.(rawData=0x%X, key=0x%X, resEA=0x%X)", rawData, key, resEA);
		return NULL;
	}

	*resEA = 0;
	val = rawData->values;
	while (val)
	{
		if (0 == strcmp(val->name, key))
		{
			break;
		}
		val = val->next;
	}

	if (val)
	{
		size_t dimensionSize = calcDimTotal(val->arr.dimensions);
		size_t dimensions[3] = { dimensionSize, 0, 0 };

		ESLOG(LOG_DEBUG, "Dimension size in the Payload : %d", dimensionSize);
		// [Jay] This is testing code only... will be removed...
		bool findValue = OCRepPayloadGetPropObjectArray(rawData,
														key,
														&payloadValues,
														dimensions);

		if (findValue == true)
		{
			*resEA = dimensionSize;
			ESLOG(LOG_DEBUG, "Found Object array payload contents.(payloadValues=0X%X, EA=%d)", payloadValues, *resEA);
		}
		else
		{
			ESLOG(LOG_ERROR, "Not Found Object array payload contents.(payloadValues=0X%X)", payloadValues);
		}
	}
	else
	{
		ESLOG(LOG_ERROR, "Not Found Object array payload contents.");
	}

	ESLOG(LOG_DEBUG, "Exit.");

	return payloadValues;
}







/*******************************************
 * CParamValue Class Function Definition.
 */

CParamValue::~CParamValue(void)
{
	ESfree(strVal);
	strVal = NULL;
}

void CParamValue::set(OCRepPayloadValue* val)
{
	if (val == NULL)
	{
		ESLOG(LOG_ERROR, "val is NULL.");
		return;
	}

	if (val->name != NULL)
	{
		type = val->type;

		switch (val->type)
		{
		case OCREP_PROP_STRING:
			ESLOG(LOG_DEBUG, "value type is OCREP_PROP_STRING.");
			strVal = ESstrdup(val->str);
			break;
		case OCREP_PROP_INT:
			ESLOG(LOG_DEBUG, "value type is OCREP_PROP_INT.");
			intVal = (int)(val->i);
			break;
		default:
			ESLOG(LOG_ERROR, "value type is Not-Supported.(%d)", val->type);
			break;
		}
	}
}

PARAMVAL_DATATYPE CParamValue::get(char* valStr, int* valInt)
{
	PARAMVAL_DATATYPE DataType = VALTYPE_NULL;

	switch (type)
	{
	case OCREP_PROP_STRING:
		ESLOG(LOG_DEBUG, "value type is OCREP_PROP_STRING.");
		if (valStr == NULL)
		{
			ESLOG(LOG_ERROR, "valStr is NULL.");
			return DataType;
		}
		valStr = ESstrdup(strVal);
		DataType = VALTYPE_STRING;
		break;
	case OCREP_PROP_INT:
		ESLOG(LOG_DEBUG, "value type is OCREP_PROP_INT.");
		if (valInt == NULL)
		{
			ESLOG(LOG_ERROR, "valInt is NULL.");
			return DataType;
		}
		*valInt = intVal;
		DataType = VALTYPE_INTEGER;
		break;
	default:
		ESLOG(LOG_ERROR, "value type is Not-Supported.(%d)", type);
		break;
	}

	return DataType;
}