// TasksView.h : interface of the CTasksView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "AppModel.h"

enum {
	UM_LISTCHANGED = WM_USER, // wParam - server's session ID, lParam - unused
	UM_REFRESHSTART,          // wParam - server's session ID, lParam - unused
	UM_REFRESHSTOP,           // wParam - server's session ID, lParam - unused
	UM_PROGRESS,              // wParam - server's session ID, lParam - ProgressInfo*
};

struct ProgressInfo {
	uint64_t content;
	uint64_t loaded;
	bool calculable;
};

using CTasksViewWinTraits = CWinTraits<WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_COMPOSITED>;
class CTasksView : public CWindowImpl<CTasksView, CWindow, CTasksViewWinTraits>
{
	using CViewSuper = CWindowImpl<CTasksView, CWindow, CTasksViewWinTraits>;

public:
	struct ServerInfo {
		std::shared_ptr<jira::server> m_server;
		std::shared_ptr<jira::server_listener> m_listener;
		std::shared_ptr<jira::report> m_dataset;
		uint32_t m_sessionId;

		ProgressInfo m_progress;
		bool m_loading = false;
		bool m_gotProgress = false;
		// TODO : relation to UI element
		std::shared_ptr<jira::node> m_plaque;

		ServerInfo(const std::shared_ptr<jira::server>& server, const std::shared_ptr<jira::server_listener>& listener, HWND hWnd);
		~ServerInfo();
		ServerInfo(const ServerInfo&) = delete;
		ServerInfo& operator=(const ServerInfo&) = delete;
		ServerInfo(ServerInfo&&) = default;
		ServerInfo& operator=(ServerInfo&&) = default;
	};

private:
	std::shared_ptr<CAppModelListener> m_listener;
	std::vector<ServerInfo> m_servers;
	std::shared_ptr<IJiraNode> m_cheatsheet;
	std::vector<ServerInfo>::iterator find(uint32_t sessionId);

	std::vector<ServerInfo>::iterator insert(std::vector<ServerInfo>::const_iterator it, const std::shared_ptr<jira::server>& server);
	std::vector<ServerInfo>::iterator erase(std::vector<ServerInfo>::const_iterator it);

	CFontHandle m_font;
	CBrush m_background;
	CCursor m_cursorObj;
	CWindow m_tooltip;

	int m_mouseX = 0;
	int m_mouseY = 0;
	std::shared_ptr<jira::node> m_hovered;
	std::shared_ptr<jira::node> m_active;
	bool m_tracking = false;
	gui::cursor m_cursor = gui::cursor::arrow;
	std::function<void(size_t, size_t)> m_scroller;

	void updateLayout();
	void updateCursor(bool force = false);
	void updateTooltip(bool force = false);
	void updateCursorAndTooltip(bool force = false);
	std::shared_ptr<jira::node> nodeFromPoint();
	void setDocumentSize(size_t width, size_t height); 
public:
	std::shared_ptr<CAppModel> m_model;

	static ATL::CWndClassInfo& GetWndClassInfo()
	{
		static ATL::CWndClassInfo wc =
		{
			{ sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, StartWindowProc,
			  0, 0, NULL, NULL, NULL, NULL, NULL, _T("TasksList"), NULL },
			NULL, NULL, NULL, TRUE, 0, _T("")
		};
		return wc;
	}

	BOOL PreTranslateMessage(MSG* pMsg);

	BEGIN_MSG_MAP(CTasksView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_SETFONT, OnSetFont)
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove);
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnMouseDown);
		MESSAGE_HANDLER(WM_LBUTTONUP, OnMouseUp);
		MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
		MESSAGE_HANDLER(UM_LISTCHANGED, OnListChanged)
		MESSAGE_HANDLER(UM_REFRESHSTART, OnRefreshStart)
		MESSAGE_HANDLER(UM_REFRESHSTOP, OnRefreshStop)
		MESSAGE_HANDLER(UM_PROGRESS, OnProgress)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSetFont(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnMouseDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnMouseUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSetCursor(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnListChanged(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnRefreshStart(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnRefreshStop(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnProgress(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);

	template <typename T>
	void setScroller(T&& scroller) {
		m_scroller = scroller;
	}
};
