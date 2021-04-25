#pragma once

#include <Windows.h>

typedef enum MsgSig
{
	MsgSig_end = 0,     // [marks end of message map]

	MsgSig_b_D_v,				// BOOL (CDC*)
	MsgSig_b_b_v,				// BOOL (BOOL)
	MsgSig_b_u_v,				// BOOL (UINT)
	MsgSig_b_h_v,				// BOOL (HANDLE)
	MsgSig_b_W_uu,				// BOOL (CWnd*, UINT, UINT)
	MsgSig_b_W_COPYDATASTRUCT,				// BOOL (CWnd*, COPYDATASTRUCT*)
	MsgSig_b_v_HELPINFO,		// BOOL (LPHELPINFO);
	MsgSig_CTLCOLOR,			// HBRUSH (CDC*, CWnd*, UINT)
	MsgSig_CTLCOLOR_REFLECT,	// HBRUSH (CDC*, UINT)
	MsgSig_i_u_W_u,				// int (UINT, CWnd*, UINT)  // ?TOITEM
	MsgSig_i_uu_v,				// int (UINT, UINT)
	MsgSig_i_W_uu,				// int (CWnd*, UINT, UINT)
	MsgSig_i_v_s,				// int (LPTSTR)
	MsgSig_l_w_l,				// LRESULT (WPARAM, LPARAM)
	MsgSig_l_uu_M,				// LRESULT (UINT, UINT, CMenu*)
	MsgSig_v_b_h,				// void (BOOL, HANDLE)
	MsgSig_v_h_v,				// void (HANDLE)
	MsgSig_v_h_h,				// void (HANDLE, HANDLE)
	MsgSig_v_v_v,				// void ()
	MsgSig_v_u_v,				// void (UINT)
	MsgSig_v_up_v,				// void (UINT_PTR)
	MsgSig_v_u_u,				// void (UINT, UINT)
	MsgSig_v_uu_v,				// void (UINT, UINT)
	MsgSig_v_v_ii,				// void (int, int)
	MsgSig_v_u_uu,				// void (UINT, UINT, UINT)
	MsgSig_v_u_ii,				// void (UINT, int, int)
	MsgSig_v_u_W,				// void (UINT, CWnd*)
	MsgSig_i_u_v,				// int (UINT)
	MsgSig_u_u_v,				// UINT (UINT)
	MsgSig_b_v_v,				// BOOL ()
	MsgSig_v_w_l,				// void (WPARAM, LPARAM)
	MsgSig_MDIACTIVATE,			// void (BOOL, CWnd*, CWnd*)
	MsgSig_v_D_v,				// void (CDC*)
	MsgSig_v_M_v,				// void (CMenu*)
	MsgSig_v_M_ub,				// void (CMenu*, UINT, BOOL)
	MsgSig_v_W_v,				// void (CWnd*)
	MsgSig_v_v_W,				// void (CWnd*)
	MsgSig_v_W_uu,				// void (CWnd*, UINT, UINT)
	MsgSig_v_W_p,				// void (CWnd*, CPoint)
	MsgSig_v_W_h,				// void (CWnd*, HANDLE)
	MsgSig_C_v_v,				// HCURSOR ()
	MsgSig_ACTIVATE,			// void (UINT, CWnd*, BOOL)
	MsgSig_SCROLL,				// void (UINT, UINT, CWnd*)
	MsgSig_SCROLL_REFLECT,		// void (UINT, UINT)
	MsgSig_v_v_s,				// void (LPTSTR)
	MsgSig_v_u_cs,				// void (UINT, LPCTSTR)
	MsgSig_OWNERDRAW,			// void (int, LPTSTR) force return TRUE
	MsgSig_i_i_s,				// int (int, LPTSTR)
	MsgSig_u_v_p,				// UINT (CPoint)
	MsgSig_u_v_v,				// UINT ()
	MsgSig_v_b_NCCALCSIZEPARAMS,		// void (BOOL, NCCALCSIZE_PARAMS*)
	MsgSig_v_v_WINDOWPOS,			// void (WINDOWPOS*)
	MsgSig_v_uu_M,				// void (UINT, UINT, HMENU)
	MsgSig_v_u_p,				// void (UINT, CPoint)
	MsgSig_SIZING,				// void (UINT, LPRECT)
	MsgSig_MOUSEWHEEL,			// BOOL (UINT, short, CPoint)
	MsgSig_MOUSEHWHEEL,			// void (UINT, short, CPoint)
	MsgSigCmd_v,				// void ()
	MsgSigCmd_b,				// BOOL ()
	MsgSigCmd_RANGE,			// void (UINT)
	MsgSigCmd_EX,				// BOOL (UINT)
	MsgSigNotify_v,				// void (NMHDR*, LRESULT*)
	MsgSigNotify_b,				// BOOL (NMHDR*, LRESULT*)
	MsgSigNotify_RANGE,			// void (UINT, NMHDR*, LRESULT*)
	MsgSigNotify_EX,			// BOOL (UINT, NMHDR*, LRESULT*)
	MsgSigCmdUI,				// void (CCmdUI*)
	MsgSigCmdUI_RANGE,			// void (CCmdUI*, UINT)
	MsgSigCmd_v_pv,				// void (void*)
	MsgSigCmd_b_pv,				// BOOL (void*)
	MsgSig_l,				// LRESULT ()
	MsgSig_l_p,				// LRESULT (CPOINT)	
	MsgSig_u_W_u,				// UINT (CWnd*, UINT)
	MsgSig_v_u_M,				// void (UINT, CMenu* )
	MsgSig_u_u_M,				// UINT (UINT, CMenu* )
	MsgSig_u_v_MENUGETOBJECTINFO,		// UINT (MENUGETOBJECTINFO*)
	MsgSig_v_M_u,				// void (CMenu*, UINT)
	MsgSig_v_u_LPMDINEXTMENU,		// void (UINT, LPMDINEXTMENU)
	MsgSig_APPCOMMAND,			// void (CWnd*, UINT, UINT, UINT)
	MsgSig_RAWINPUT,			// void (UINT, HRAWINPUT)
	MsgSig_u_u_u,				// UINT (UINT, UINT)
	MsgSig_MOUSE_XBUTTON,			// void (UINT, UINT, CPoint)
	MsgSig_MOUSE_NCXBUTTON,			// void (short, UINT, CPoint)
	MsgSig_INPUTLANGCHANGE,			// void (UINT, UINT)
	MsgSig_v_u_hkl,					// void (UINT, HKL)
	MsgSig_INPUTDEVICECHANGE,		// void (unsigned short, HANDLE)
	MsgSig_l_D_u,					// LRESULT (CDC*, UINT)
	MsgSig_i_v_S,				// int (LPCTSTR)
	MsgSig_v_F_b,				// void (CFont*, BOOL)
	MsgSig_h_v_v,				// HANDLE ()
	MsgSig_h_b_h,				// HANDLE (BOOL, HANDLE)
	MsgSig_b_v_ii,				// BOOL (int, int)
	MsgSig_h_h_h,				// HANDLE (HANDLE, HANDLE)
	MsgSig_MDINext,				// void (CWnd*, BOOL)
	MsgSig_u_u_l,				// UINT (UINT, LPARAM)
} MsgSig;

#define PASCAL      __stdcall

#include "vCIoTREQParsor.h"

class CMessageHandler : public vCIoTReqParsor
{
public :
	typedef void (CMessageHandler::* PMSGFunc)(void);

	typedef struct MSG_MSGMAP_ENTRY
	{
		UINT nMessage;   // windows message
		UINT nCode;      // control code or WM_NOTIFY code
		UINT nID;        // control ID (or 0 for windows messages)
		UINT nLastID;    // used for entries specifying a range of control id's
		UINT_PTR nSig;   // signature type (action) or pointer to message #
		PMSGFunc pfn;    // routine to call (or special value)
	} MSG_MSGMAP_ENTRY;

	CMessageHandler(void) {};
	
	virtual ~CMessageHandler(void) { printf("CMessageHandler class: Defualt-Terminate Function.\n");  };

	virtual const MSG_MSGMAP_ENTRY* GetMessageMap(void) const = 0;

	virtual bool IoT_ReqParse(REQ_CMDLIST method, void* data, TransactionDataType transNum) = 0;

	bool ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam);

private :
	bool RunMessageFunc(UINT_PTR funcType, PMSGFunc pFunc, WPARAM wParam, LPARAM lParam);
};



//struct MSG_MSGMAP
//{
//	const MSG_MSGMAP* (PASCAL* pfnGetBaseMap)(void);
//	const MSG_MSGMAP_ENTRY* lpEntries;
//};


#define DECLARE_MESSAGE_MAP() \
protected: \
	static const CMessageHandler::MSG_MSGMAP_ENTRY* PASCAL GetThisMessageMap(); \
	virtual const CMessageHandler::MSG_MSGMAP_ENTRY* GetMessageMap() const; \


#define BEGIN_MESSAGE_MAP(theClass) \
	__pragma(warning(push))\
	__pragma(warning(disable : 4867))\
	const CMessageHandler::MSG_MSGMAP_ENTRY* theClass::GetMessageMap() const \
		{ return GetThisMessageMap(); } \
	const CMessageHandler::MSG_MSGMAP_ENTRY* PASCAL theClass::GetThisMessageMap() \
	{ \
		typedef theClass ThisClass;						   \
		static const CMessageHandler::MSG_MSGMAP_ENTRY _messageEntries[] =  \
		{

#define END_MESSAGE_MAP() \
			{0, 0, 0, 0, MsgSig_end, (CMessageHandler::PMSGFunc)0 } \
		}; \
		return _messageEntries; \
	}								  \
	__pragma(warning( pop ))


#define ON_MESSAGE(message, memberFxn) \
			{ message, 0, 0, 0, MsgSig_l_w_l, ((CMessageHandler::PMSGFunc)(memberFxn)) },

