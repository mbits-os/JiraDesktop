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

class CTasksView : public CWindowImpl<CTasksView>
{
	struct ServerInfo {
		std::shared_ptr<jira::server> m_server;
		std::shared_ptr<jira::server_listener> m_listener;
		std::shared_ptr<jira::report> m_dataset;
		uint32_t m_sessionId;
		// TODO : relation to UI element

		ServerInfo(const std::shared_ptr<jira::server>& server, const std::shared_ptr<jira::server_listener>& listener)
			: m_server(server)
			, m_listener(listener)
			, m_sessionId(server->sessionId())
		{
			m_server->registerListener(m_listener);
		}

		~ServerInfo()
		{
			if (m_server && m_listener)
				m_server->unregisterListener(m_listener);
		}

		ServerInfo(const ServerInfo&) = delete;
		ServerInfo& operator=(const ServerInfo&) = delete;
		ServerInfo(ServerInfo&&) = default;
		ServerInfo& operator=(ServerInfo&&) = default;
	};

	std::shared_ptr<CAppModelListener> m_listener;
	std::vector<ServerInfo> m_servers;
	std::vector<ServerInfo>::iterator find(uint32_t sessionId);

	std::vector<ServerInfo>::iterator insert(std::vector<ServerInfo>::const_iterator it, const std::shared_ptr<jira::server>& server);
	std::vector<ServerInfo>::iterator erase(std::vector<ServerInfo>::const_iterator it);
public:
	std::shared_ptr<CAppModel> m_model;

	DECLARE_WND_CLASS(NULL)

	BOOL PreTranslateMessage(MSG* pMsg);

	BEGIN_MSG_MAP(CTasksView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
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
	LRESULT OnListChanged(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnRefreshStart(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnRefreshStop(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnProgress(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
};
