// TasksFrame.h : interface of the CTasksFrame class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "TaskBarIcon.h"
#include "AppModel.h"

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
{
	using CFameSuper = CFrameWindowImpl<CTasksFrame, CWindow, CTasksFrameWinTraits>;
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("TasksWindow"), IDR_MAINFRAME, 0, COLOR_WINDOW)

	CTasksView m_view;
	CScrollContainerEx m_container;
	CTaskBarIcon m_taskIcon;
	CFont m_font;
	std::shared_ptr<CAppModel> m_model = std::make_shared<CAppModel>();

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle();

	BEGIN_UPDATE_UI_MAP(CTasksFrame)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CTasksFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		TASKBAR_MESSAGE_HANDLER(m_taskIcon, WM_LBUTTONDOWN, OnTaskIconClick)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_FILE_NEW, OnFileNew)
		COMMAND_ID_HANDLER(ID_TASKS_REFRESH, OnTasksRefersh)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
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
	LRESULT OnTaskIconClick(LPARAM /*uMsg*/, BOOL& /*bHandled*/);
	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnTasksRefersh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void newConnection();
	void refreshAll();
	void exitApplication();
	void showLicence();
	void about();
};
