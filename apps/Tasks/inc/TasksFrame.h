// TasksFrame.h : interface of the CTasksFrame class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "TaskBarIcon.h"
#include "AppModel.h"
#include "TasksActions.h"

class CScrollContainerEx : public CScrollContainerImpl<CScrollContainerEx>
{
public:
	DECLARE_WND_CLASS_EX(_T("ScrollContainerView"), 0, -1)
};

using CTasksFrameWinTraits = CWinTraits<WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_APPWINDOW>;
class CTasksFrame
	: public CFrameWindowImpl<CTasksFrame, CWindow, CTasksFrameWinTraits>
	, public CUpdateUI<CTasksFrame>
	, public CMessageFilter
	, public CIdleHandler
	, public CTasksActions<CTasksFrame>
{
	using CFameSuper = CFrameWindowImpl<CTasksFrame, CWindow, CTasksFrameWinTraits>;
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("TasksWindow"), IDR_MAINFRAME, 0, COLOR_WINDOW)

	CTasksView m_view;
	CScrollContainerEx m_container;
	CTaskBarIcon m_taskIcon;
	CFont m_font;
	CIcon m_bigIcon;
	CIcon m_smallIcon;
	std::shared_ptr<CAppModel> m_model = std::make_shared<CAppModel>();

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle();

	void rebuildAccel();

	BEGIN_UPDATE_UI_MAP(CTasksFrame)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CTasksFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_COMMAND, OnCommand)
		NOTIFY_CODE_HANDLER(TTN_GETDISPINFOA, OnToolTipTextA)
		NOTIFY_CODE_HANDLER(TTN_GETDISPINFOW, OnToolTipTextW)
		TASKBAR_MESSAGE_HANDLER(m_taskIcon, WM_LBUTTONDOWN, OnTaskIconClick)
		CHAIN_MSG_MAP(CUpdateUI<CTasksFrame>)
		CHAIN_MSG_MAP(CFameSuper)
		CHAIN_MSG_MAP_MEMBER(m_taskIcon)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnToolTipTextA(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnToolTipTextW(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnTaskIconClick(LPARAM /*uMsg*/, BOOL& /*bHandled*/);

	void newConnection();
	void refreshAll();
	void exitApplication();
	void showLicence();
	void about();
};
