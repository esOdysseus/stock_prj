#ifndef _STOCK_SEARCH_H_EUNSEOK_KIM_
#define _STOCK_SEARCH_H_EUNSEOK_KIM_

#include "Config.h"
#include <pthread.h>

// Received Data Ȥ�� Real Received Data ������ ���� Header.
//#include "ChartExcel.h"		// Excel Chart���� ������ ��ȸ.
//#include "ChartIndex.h"		// Index Chart���� ������ ��ȸ. (��뿡 ��������.)
                                // ChartCtrl.dll ���� �ε�� Window â ���� ���� ����� �ִµ� ����.
//#include "t4201.h"			// �ֽ�íƮ(����) �ϴ��� Test�� ���, �д��� Test �ȵ�.

// Received Data ������ ���� Header.
#include "t0167.h"			// �ð���ȸ
#include "t1102.h"			// �ֽ� ���簡(�ü�) ��ȸ
#include "t1305.h"			// �Ⱓ�� �ְ�
#include "t8411.h"			// �ֽ�íƮ(ƽ/nƽ)
#include "t8412.h"			// �ֽ�íƮ(N��)
#include "t8413.h"			// �ֽ�íƮ(���ֿ�)
#include "t8430.h"			// �ֽ�������ȸ

// Real Received Data ������ ���� Header.
#include "CSPAQ12300.h"		// �������� �ܰ��� ��ȸ.
#include "CSPAQ13700.h"		// �������� �ֹ�ü�᳻�� ��ȸ.
#include "CSPAT00600.h"		// ���� �����ֹ�.
#include "CSPAT00700.h"		// ���� �����ֹ�.
#include "CSPAT00800.h"		// ���� ����ֹ�.
#include "H1_.h"			// KOSPIȣ���ܷ�
#include "HA_.h"			// KOSDAQȣ���ܷ�
#include "K3_.h"			// KOSDAQü��
#include "S3_.h"			// KOSPIü��
#include "SC0.h"			// �ֽ��ֹ�����
#include "SC1.h"			// �ֽ��ֹ�ü��
#include "SC2.h"			// �ֽ��ֹ�����
#include "SC3.h"			// �ֽ��ֹ����
#include "SC4.h"			// �ֽ��ֹ��ź�
#include "JIF.h"			// ������

/***********************************************
*
*	"�ֽ���Ʈ(����)"�� ���� Definition.
*	- Mode : Received-Data Event
*	- �д��� Server ���� : ���� ������ ���� 80�������� ����.
*	- ƽ���� Server ���� : ���� ������ ���� 10�������� ����.
*
***********************************************/
#define TRCODE_CHART_TICKSEARCH    NAME_t8411
#define TRCODE_CHART_MINSEARCH     NAME_t8412
#define TRCODE_CHART_DAYSSEARCH    NAME_t8413

#define DELAY_CHART_SEARCH		1000	// 1 second

typedef enum STEnumPeriod {
	STE_PRID_SOF = 0,
	STE_PRID_1TICK = 1,	// tick '0'
	STE_PRID_10TICK = 2,
	STE_PRID_100TICK = 3,
	STE_PRID_1MIN = 4,	// min '1'
	STE_PRID_10MIN = 5,
	STE_PRID_30MIN = 6,
	STE_PRID_1HOUR = 7,	// min '1'
	STE_PRID_3HOUR = 8,
	STE_PRID_1DAY = 9,	// day '2'
	STE_PRID_2DAY = 10,
	STE_PRID_10DAY = 11,
	STE_PRID_1WEEK = 12,	// week '3'
	STE_PRID_2WEEK = 13,
	STE_PRID_3WEEK = 14,
	STE_PRID_1MON = 15,	// mon '4'
	STE_PRID_3MON = 16,
	STE_PRID_6MON = 17,
	STE_PRID_1YEAR = 18,	// mon '4'
	STE_PRID_EOF
} STE_PERIOD;

#define STChartTickSearchOut1Name	NAME_t8411OutBlock
#define STChartTickSearchOut2Name	NAME_t8411OutBlock1
// Tick �⺻�Է�
typedef struct _t8411InBlock		STChartTickSearchIn, *pSTChartTickSearchIn;
// Tick ���1                          , occurs
typedef struct _t8411OutBlock		STChartTickSearchOut1, *pSTChartTickSearchOut1;
// Tick ���2                          , occurs
typedef struct _t8411OutBlock1		STChartTickSearchOut2, *pSTChartTickSearchOut2;

#define STChartMinSearchOut1Name	NAME_t8412OutBlock
#define STChartMinSearchOut2Name	NAME_t8412OutBlock1
// Min �⺻�Է�
typedef struct _t8412InBlock		STChartMinSearchIn, *pSTChartMinSearchIn;
// Min ���1                          , occurs
typedef struct _t8412OutBlock		STChartMinSearchOut1, *pSTChartMinSearchOut1;
// Min ���2                          , occurs
typedef struct _t8412OutBlock1		STChartMinSearchOut2, *pSTChartMinSearchOut2;

#define STChartDaysSearchOut1Name	NAME_t8413OutBlock
#define STChartDaysSearchOut2Name	NAME_t8413OutBlock1
// Days �⺻�Է�
typedef struct _t8413InBlock		STChartDaysSearchIn, *pSTChartDaysSearchIn;
// Days ���1                          , occurs
typedef struct _t8413OutBlock		STChartDaysSearchOut1, *pSTChartDaysSearchOut1;
// Days ���2                          , occurs
typedef struct _t8413OutBlock1		STChartDaysSearchOut2, *pSTChartDaysSearchOut2;

/***********************************************
*
*	"�������� �ܰ��� ��ȸ"�� ���� Definition.
*	- Mode : ?
*
***********************************************/
#define TRCODE_ACTBALANCE_SEARCH     NAME_CSPAQ12300

// �⺻�Է�
typedef struct _CSPAQ12300InBlock1	STActBalSearchIn, *pSTActBalSearchIn;
// ���1                          , occurs
typedef struct _CSPAQ12300OutBlock1	STActBalSearchOut1, *pSTActBalSearchOut1;
// ���2                          , occurs
typedef struct _CSPAQ12300OutBlock2	STActBalSearchOut2, *pSTActBalSearchOut2;
// ���3                          , occurs
typedef struct _CSPAQ12300OutBlock3	STActBalSearchOut3, *pSTActBalSearchOut3;


/***********************************************
 *
 *	"�ֽ� ���� ��ȸ"�� ���� Definition.
 *	- Mode : Received-Data Event
 *
 ***********************************************/
#define TRCODE_STOCK_SEARCH     NAME_t8430

typedef enum STEnumStockSearch{
	STE_SS_ALL		= '0',
	STE_SS_KOSPI	= '1',
	STE_SS_KOSDAQ	= '2'
} STE_STOCKSEARCH;

// �⺻�Է�
typedef struct _t8430InBlock	STStockSearchIn, *pSTStockSearchIn;
// ���1                          , occurs
typedef struct _t8430OutBlock	STStockSearchOut, *pSTStockSearchOut;


/***********************************************
*
*	"Server �ð� ��ȸ"�� ���� Definition.
*	- Mode : Received-Data Event
*
***********************************************/
#define TRCODE_TIME_SEARCH     NAME_t0167

// �⺻�Է�                    
typedef struct _t0167InBlock	STTimeSearchIn, *pSTTimeSearchIn;
// ���                           
typedef struct _t0167OutBlock	STTimeSearchOut, *pSTTimeSearchOut;


/***********************************************
*
*	"Ư�� �ֽ��� ���簡 ��ȸ"�� ���� Definition.
*	- Mode : Received-Data Event
*
***********************************************/
#define TRCODE_ONESTOCK_SEARCH     NAME_t1102

// �⺻�Է�                  
typedef struct _t1102InBlock	STOneSearchIn, *pSTOneSearchIn;
// ���                           
typedef struct _t1102OutBlock	STOneSearchOut, *pSTOneSearchOut;


/***********************************************
*
*	"���� �����ֹ�"�� ���� Definition.
*	- Mode : Received-Data Event
*
***********************************************/
#define TRCODE_TRADE_ORDER     NAME_CSPAT00600

typedef enum STEnumTrade {
	STE_TRADE_SELL = 1,
	STE_TRADE_BUY  = 2
} STE_TRADE;

// In(*EMPTY*)                    
typedef struct _CSPAT00600InBlock1		STTradeOrderIn, *pSTTradeOrderIn;
// In(*EMPTY*)                    
typedef struct _CSPAT00600OutBlock1		STTradeOrderOut1, *pSTTradeOrderOut1;
// Out(*EMPTY*)                   
typedef struct _CSPAT00600OutBlock2		STTradeOrderOut2, *pSTTradeOrderOut2;








/***********************************************
*
*	"�ֽ� �ֹ� ���� �˸�"�� ���� Event Definition.
*	- Mode : Received-Real-Data Event
*
***********************************************/
#define TRCODE_ORDER_ACT     NAME_SC0

// ���                           
typedef struct _SC0_OutBlock	STOrderACTOut, *pSTOrderACTOut;

/***********************************************
*
*	"�ֽ� �ֹ� ���� �˸�"�� ���� Event Definition.
*	- Mode : Received-Real-Data Event
*
***********************************************/
#define TRCODE_ORDER_SUCCESS     NAME_SC1

// ���                           
typedef struct _SC1_OutBlock	STOrderSuccessOut, *pSTOrderSuccessOut;

/***********************************************
*
*	"�ֽ� �ֹ� ���� �˸�"�� ���� Event Definition.
*	- Mode : Received-Real-Data Event
*
***********************************************/
#define TRCODE_ORDER_CHANGED     NAME_SC2

// ���                           
typedef struct _SC2_OutBlock	STOrderChangedOut, *pSTOrderChangedOut;

/***********************************************
*
*	"�ֽ� �ֹ� ��� �˸�"�� ���� Event Definition.
*	- Mode : Received-Real-Data Event
*
***********************************************/
#define TRCODE_ORDER_CANCEL     NAME_SC3

// ���                           
typedef struct _SC3_OutBlock	STOrderCancelOut, *pSTOrderCancelOut;

/***********************************************
*
*	"�ֽ� �ֹ� �ź� �˸�"�� ���� Event Definition.
*	- Mode : Received-Real-Data Event
*
***********************************************/
#define TRCODE_ORDER_REJECTED     NAME_SC4

// ���                           
typedef struct _SC4_OutBlock	STOrderRejectedOut, *pSTOrderRejectedOut;





/***********************************************
*
*	"KOSPI �ֹ� ���� �˸�"�� ���� Event Definition.
*	- Mode : Received-Real-Data Event
*
***********************************************/
#define TRCODE_KOSPI_SUCCESS     NAME_S3_

// Observe Register Input
typedef struct _S3__InBlock		STkospiSuccessIn, *pSTkospiSuccessIn;
// ���                           
typedef struct _S3__OutBlock		STkospiSuccessOut, *pSTkospiSuccessOut;

/***********************************************
*
*	"KOSDAQ �ֹ� ���� �˸�"�� ���� Event Definition.
*	- Mode : Received-Real-Data Event
*
***********************************************/
#define TRCODE_KOSDAQ_SUCCESS     NAME_K3_

// Observe Register Input
typedef struct _K3__InBlock		STkosdaqSuccessIn, *pSTkosdaqSuccessIn;
// ���                           
typedef struct _K3__OutBlock		STkosdaqSuccessOut, *pSTkosdaqSuccessOut;

/***********************************************
*
*	"KOSPI ȣ���ܷ� �˸�"�� ���� Event Definition.
*	- Mode : Received-Real-Data Event
*
***********************************************/
#define TRCODE_KOSPI_REMAIN     NAME_H1_

// Observe Register Input
typedef struct _H1__InBlock		STkospiRemainOrderIn, *pSTkospiRemainOrderIn;
// ���                           
typedef struct _H1__OutBlock		STkospiRemainOrderOut, *pSTkospiRemainOrderOut;

/***********************************************
*
*	"KOSDAQ ȣ���ܷ� �˸�"�� ���� Event Definition.
*	- Mode : Received-Real-Data Event
*
***********************************************/
#define TRCODE_KOSDAQ_REMAIN     NAME_HA_

// Observe Register Input
typedef struct _HA__InBlock		STkosdaqRemainOrderIn, *pSTkosdaqRemainOrderIn;
// ���                           
typedef struct _HA__OutBlock		STkosdaqRemainOrderOut, *pSTkosdaqRemainOrderOut;





#include <map>
#include <list>
#include <string>
#include <sREQ.h>

/**
 * @brief  Managing-Class both sending request and handling response with regard to Stock.
 */
class CSTReqResManager
{
private:
	typedef int (CSTReqResManager::*handlerFuncType)(sREQ* request, char* dataType, void* data1, void* data2);
	typedef int (CSTReqResManager::*handlerRealFuncType)(void* data1, void* data2);
	typedef int (*handlerGFuncType)(void* data1, void* data2, void* data3);
	typedef std::map<std::string, handlerGFuncType>		TOrderMapType;
	typedef std::list<sREQ*>						PenReqMapType;
	typedef std::map<int, sREQ*>					SentReqMapType;

	void* xingAPIins;
	char* accountNum;
	char* passNum;
	//bool isAllowTradeOrder;

	std::map<std::string, handlerFuncType> handlerDataMap;		// for Received-Data Event.
	std::map<std::string, handlerRealFuncType> handlerRealDataMap;	// for Received-Real-Data Event.
	PenReqMapType PendingRequestMap;
	SentReqMapType SentRequestMap;		// key: reqID , value: req

public:
	CSTReqResManager(void);

	CSTReqResManager(void* xingIns);

	~CSTReqResManager(void);

	bool AllowTradeOrder(const char* account, const char* passwd);

	int CallDataHandler(int reqID, char* dataType, char* trCode, void* data1, void* data2);

	int CallRealDataHandler(char* trCode, void* data1, void* data2);

	bool SendPendingRequest(void);

	void RelPendingRequest(int reqID);

	int SendStockSearch(STE_STOCKSEARCH market);

	int SendTimeSearch(void);

	int SendOneStockSearch(const char* stockCode);

	int SendTradeOrder(STE_TRADE mode, const char* stockCode, const unsigned int stockEA, const unsigned int pricePerStock);

	int SendChartSearch(const char* stockCode, STE_PERIOD period,
					int sYear, int sMon, int sDay, int eYear, int eMon, int eDay,
					int sHour=0, int sMin = 0, int sSec = 0, int eHour = 0, int eMin = 0, int eSec = 0);

	int SendActBalanceSearch(void);

private:
	typedef enum _ObservorStater_
	{
		STE_OBVST_NONE = 0,
		STE_OBVST_DEINITED = 1,
		STE_OBVST_DEINITING = 2,
		STE_OBVST_INITED = 3,
		STE_OBVST_ORDERED = 4
	} STE_OBVORSTATE_TRADE;

	STE_OBVORSTATE_TRADE TradeState;
	pthread_mutex_t Mutex_TradeState;
	TOrderMapType handlerStockOBVMap;
	pthread_mutex_t Mutex_StockOBVMap;

	// handler Function Declation for Received-Data Event.
	int handleStockSearch(sREQ* request, char* dataType, STStockSearchOut* data, int dataSize);
	int handleTimeSearch(sREQ* request, char* dataType, STTimeSearchOut* data, int dataSize);
	int handleOneStockSearch(sREQ* request, char* dataType, STOneSearchOut* data, int dataSize);
	int handleTradeOrder(sREQ* request, char* dataType, void* data, int dataSize);
	int handleChartSearch(sREQ* request, char* dataType, void* data, int dataSize);
	int handleActBalanceSearch(sREQ* request, char* dataType, void* data, int dataSize);

	// Observor Register/Unregister Function Declation. for Receiving Real-Data Event.
	bool RegisterObservers(const char* stockCode);
	bool unRegisterObservers(STE_OBVORSTATE_TRADE targetState, const char* stockCode);
	bool registerTOrderStockOBV(const char* stockCode);
	bool unregisterTOrderStockOBV(bool isAllOBVs, const char* stockCode);

	// handler Function Declation for Received-Real-Data Event. (Occurred by Observor-register.)
	int handleDummy(void* data, int dataSize);
	int handleTOrdSuccess(STOrderSuccessOut* data, int dataSize);
	int handleTOrdChanged(STOrderChangedOut* data, int dataSize);
	int handleTOrdCancel(STOrderCancelOut* data, int dataSize);
	int handleTOrdRejected(STOrderRejectedOut* data, int dataSize);
	int handleKPISuccess(STkospiSuccessOut* data, int dataSize);
	int handleKDAQSuccess(STkosdaqSuccessOut* data, int dataSize);
	int handleKPIRemain(STkospiRemainOrderOut* data, int dataSize);
	int handleKDAQRemain(STkosdaqRemainOrderOut* data, int dataSize);
};


#endif // _STOCK_SEARCH_H_EUNSEOK_KIM_















