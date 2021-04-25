#ifndef _IOTREQUEST_PARSOR_H_SUB_CLASS_EUNSEOK_KIM_
#define _IOTREQUEST_PARSOR_H_SUB_CLASS_EUNSEOK_KIM_

#include <octypes.h>
#include <map>
#include <string>

/**
 *
 * IoT request Parsing Class.
 * 
 * Support Message format
 * {cmd:"majorCmd",param:[],minorea:2,minor:[{cmd:"minorCmd01",param:[]},{cmd:"minorCmd02",param:[]}]}
 *
 */
typedef enum {
	VALTYPE_NULL = 0,
	VALTYPE_STRING = 1,
	VALTYPE_INTEGER = 2
} PARAMVAL_DATATYPE;

class CParamValue {
private:
	OCRepPayloadPropType type;
	char* strVal;
	int intVal;

public:
	CParamValue(void) {};
	~CParamValue(void);

	void set(OCRepPayloadValue* val);
	PARAMVAL_DATATYPE get(char* valStr, int* valInt);
};

class iotReqDecoder
{
public:
	typedef std::map<std::string, CParamValue> ParamMap;

private:
	typedef struct sCmd {
		char* Name;
		ParamMap Params;
	}sCmd;

	sCmd majorCmd;

	int minorCmdEA;
	sCmd* minorCmd;
	bool parsingSuccess;

private :
	bool parsing(const OCRepPayload* rawData);

public :
	iotReqDecoder(OCPayload* rawData);

	~iotReqDecoder(void);

	bool GetResultParsing(void);

	char* GetMajorCmd(void);

	ParamMap* GetMajorParams(void);

	void* GetMajorParamVal(const char* key, PARAMVAL_DATATYPE* dataType);

	int GetMinorCmdEA(void);

	char* GetMinorCmd(int index);

	ParamMap* GetMinorParams(int index);

	void* GetMinorParamVal(int index, const char* key, PARAMVAL_DATATYPE* dataType);
};


#endif // _IOTREQUEST_PARSOR_H_SUB_CLASS_EUNSEOK_KIM_