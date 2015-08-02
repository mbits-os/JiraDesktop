// TasksFrame.h : interface of the CTasksFrame class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "TaskBarIcon.h"
#include "AppModel.h"
#include "TasksActions.h"
#include "langs.h"

class CScrollContainerEx : public CScrollContainerImpl<CScrollContainerEx>
{
public:
	DECLARE_WND_CLASS_EX(_T("ScrollContainerView"), 0, -1)

	BEGIN_MSG_MAP(CScrollWindowImpl)
		MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
		CHAIN_MSG_MAP(CScrollContainerImpl<CScrollContainerEx>)
	END_MSG_MAP()

	int m_zoomDelta = 0;

	LRESULT OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if (LOWORD(wParam) & MK_CONTROL) {
#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400) || defined(_WIN32_WCE)
			uMsg;
			int zDelta = (int)GET_WHEEL_DELTA_WPARAM(wParam);
#else
			int zDelta = (uMsg == WM_MOUSEWHEEL) ? (int)GET_WHEEL_DELTA_WPARAM(wParam) : (int)wParam;
#endif // !((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400) || defined(_WIN32_WCE))

			m_zoomDelta += zDelta;   // cumulative
			int levels = m_zoomDelta / WHEEL_DELTA;
			m_zoomDelta %= WHEEL_DELTA;

			if (levels < 0) {
				levels = -levels;
				zoomOut(levels);
			} else {
				zoomIn(levels);
			}

			return 0;
		}

		bHandled = FALSE;
		return 0;
	}

	void zoomIn(int level) {
		::SendMessage(m_wndClient, AM_ZOOM, (WPARAM)level, 0);
	}

	void zoomOut(int level) {
		::SendMessage(m_wndClient, AM_ZOOM, (WPARAM)level, 1);
	}
};

using CTasksFrameWinTraits = CWinTraits<WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_APPWINDOW>;
class CTasksFrame
	: public CFrameWindowImpl<CTasksFrame, CWindow, CTasksFrameWinTraits>
	, public CUpdateUI<CTasksFrame>
	, public CMessageFilter
	, public CIdleHandler
	, public CTasksActions<CTasksFrame>
	, public Scroller
{
	using CFameSuper = CFrameWindowImpl<CTasksFrame, CWindow, CTasksFrameWinTraits>;
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("TasksWindow"), IDR_MAINFRAME, 0, COLOR_WINDOW)

	CTasksView m_view;
	CScrollContainerEx m_container;
	CTaskBarIcon m_taskIcon;
	CTaskBarIcon m_attentionIcon;
	CFont m_font;
	CIcon m_bigIcon;
	CIcon m_smallIcon;
	std::shared_ptr<CAppModel> m_model = std::make_shared<CAppModel>();
	bool m_balloonVisible = false;
	Tasks::Strings _;

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle();

	void rebuildAccel();
	bool updatePosition();

	BEGIN_UPDATE_UI_MAP(CTasksFrame)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CTasksFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_COMMAND, OnCommand)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
		NOTIFY_CODE_HANDLER(TTN_GETDISPINFOW, OnToolTipTextW)
		NOTIFY_CODE_HANDLER(TTN_GETDISPINFOA, OnToolTipTextA)
		NOTIFY_CODE_HANDLER(TTN_GETDISPINFOW, OnToolTipTextW)
#ifdef LCLIK_TASK_MENU
		TASKBAR_MESSAGE_HANDLER(m_taskIcon, WM_LBUTTONDOWN, OnTaskIconClick)
#endif
		TASKBAR_MESSAGE_HANDLER(m_taskIcon, WM_LBUTTONDBLCLK, OnTaskIconDefault)
		TASKBAR_MESSAGE_HANDLER(m_attentionIcon, WM_LBUTTONUP, OnAttentionIconClick)
		TASKBAR_MESSAGE_HANDLER(m_attentionIcon, NIN_BALLOONUSERCLICK, OnAttentionIconClick)
		TASKBAR_MESSAGE_HANDLER(m_attentionIcon, NIN_BALLOONTIMEOUT, OnAttentionIconTimeoutOrHide)
		TASKBAR_MESSAGE_HANDLER(m_attentionIcon, NIN_BALLOONHIDE, OnAttentionIconTimeoutOrHide)
		CHAIN_MSG_MAP(CUpdateUI<CTasksFrame>)
		CHAIN_MSG_MAP(CFameSuper)
		CHAIN_MSG_MAP_MEMBER(m_taskIcon)
		CHAIN_MSG_MAP_MEMBER(m_attentionIcon)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnSysCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnActivate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnToolTipTextA(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnToolTipTextW(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/);
#ifdef LCLIK_TASK_MENU
	LRESULT OnTaskIconClick(LPARAM /*uMsg*/, BOOL& /*bHandled*/);
#endif
	LRESULT OnTaskIconDefault(LPARAM /*uMsg*/, BOOL& /*bHandled*/);
	LRESULT OnAttentionIconClick(LPARAM /*uMsg*/, BOOL& /*bHandled*/);
	LRESULT OnAttentionIconTimeoutOrHide(LPARAM /*uMsg*/, BOOL& /*bHandled*/);

	void showHide();
	void newConnection();
	void refreshAll();
	void exitApplication();
	void showLicence();
	void about();

	void setContentSize(size_t width, size_t height) override;
	void scrollIntoView(long left, long top, long right, long bottom) override;
};
