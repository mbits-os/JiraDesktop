// TasksView.h : interface of the CTasksView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "AppModel.h"
#include <gui/styles.hpp>
#include <gui/win32_animation.hpp>
#include "langs.h"

enum {
	UM_LISTCHANGED = WM_USER, // wParam - server's session ID, lParam - unused
	UM_REFRESHSTART,          // wParam - views's session ID,  lParam - unused
	UM_REFRESHSTOP,           // wParam - views's session ID,  lParam - unused
	UM_PROGRESS,              // wParam - views's session ID,  lParam - ProgressInfo*
	UM_LAYOUTNEEDED,          // wParam - 0,                   lParam - 0
};

enum {
	AM_ZOOM = WM_APP + 1
};

struct ProgressInfo {
	uint64_t content;
	uint64_t loaded;
	bool calculable;
};

struct ZoomInfo {
	gui::ratio zoom;
	gui::ratio device;
	gui::ratio mul;
#ifdef DEBUG_UPDATES
	std::vector<RECT> updates;
#endif
};

// #define CAIRO_PAINTER

struct Scroller {
	virtual void setContentSize(size_t width, size_t height) = 0;
	virtual void scrollIntoView(long left, long top, long right, long bottom) = 0;
};

class DocOwner;

enum { TIMER_SCENE = 0x1001F00D };

struct text_animator_cb {
	virtual void setText(const char* value, size_t count) = 0;
};

using CTasksViewWinTraits = CWinTraits<WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_COMPOSITED>;
class CTasksView : public CWindowImpl<CTasksView, CWindow, CTasksViewWinTraits>
{
	using CViewSuper = CWindowImpl<CTasksView, CWindow, CTasksViewWinTraits>;

public:
	struct ViewInfo: text_animator_cb, std::enable_shared_from_this<ViewInfo> {
		jira::search_def m_view;
		std::shared_ptr<gui::document> m_document;
		std::shared_ptr<jira::report> m_dataset;
		std::shared_ptr<jira::report> m_previous;

		std::shared_ptr<gui::node> m_plaque;
		std::shared_ptr<gui::node> m_progressCtrl;

		std::shared_ptr<ani::animation> m_wait, m_load;

		ProgressInfo m_progress;
		bool m_loading = false;
		bool m_gotProgress = false;

		ViewInfo(const jira::search_def& view,
			const std::shared_ptr<gui::document>& doc);
		~ViewInfo();
		ViewInfo(const ViewInfo&) = delete;
		ViewInfo& operator=(const ViewInfo&) = delete;
		ViewInfo(ViewInfo&&) = default;
		ViewInfo& operator=(ViewInfo&&) = default;

		void updateProgress(ani::win32::scene& scene);
		std::shared_ptr<gui::node> buildPlaque(ani::win32::scene& scene, const Strings& tr);
		void updatePlaque(ani::win32::scene& scene, std::vector<std::string>& removed, std::vector<std::string>& modified, std::vector<std::string>& added, const Strings& tr);

	private:
		void updateDataset(ani::win32::scene& scene, std::vector<std::string>& removed, std::vector<std::string>& modified, std::vector<std::string>& added, const Strings& tr);
		std::shared_ptr<gui::node> buildSchema();
		std::shared_ptr<gui::node> createNote(const Strings& tr);
		void mergeTable(std::vector<std::string>& removed, std::vector<std::string>& modified, std::vector<std::string>& added, const Strings& tr);
		void createTable(ani::win32::scene& scene, const Strings& tr);
		void setText(const char* value, size_t count) override;
	};

	struct ServerInfo {
		std::shared_ptr<jira::server> m_server;
		std::shared_ptr<gui::document> m_document;
		std::shared_ptr<jira::server_listener> m_listener;
		std::vector<std::shared_ptr<ViewInfo>> m_views;

		std::shared_ptr<gui::node> m_plaque;

		ServerInfo(const std::shared_ptr<jira::server>& server,
			const std::shared_ptr<gui::document>& doc,
			const std::shared_ptr<jira::server_listener>& listener,
			ani::win32::scene& scene,
			const Strings& tr);
		~ServerInfo();
		ServerInfo(const ServerInfo&) = delete;
		ServerInfo& operator=(const ServerInfo&) = delete;
		ServerInfo(ServerInfo&&) = default;
		ServerInfo& operator=(ServerInfo&&) = default;

		void buildPlaque(ani::win32::scene& scene, const Strings& tr);
		void updatePlaque(ani::win32::scene& scene, std::vector<std::string>& removed, std::vector<std::string>& modified, std::vector<std::string>& added, const Strings& tr);
		std::vector<jira::search_def>::const_iterator findDefinition(uint32_t sessionId) const;
		std::vector<std::shared_ptr<ViewInfo>>::iterator findInfo(uint32_t sessionId);
	private:
		void updateErrors();
	};

private:
	std::shared_ptr<CAppModelListener> m_listener;
	std::vector<ServerInfo> m_servers;
	std::shared_ptr<gui::node> m_body;
	std::shared_ptr<DocOwner> m_docOwner;
	std::vector<ServerInfo>::iterator findServer(uint32_t sessionId);
	std::vector<ServerInfo>::iterator findViewServer(uint32_t sessionId);

	std::vector<ServerInfo>::iterator insert(std::vector<ServerInfo>::const_iterator it, const ::ServerInfo& info);
	std::vector<ServerInfo>::iterator erase(std::vector<ServerInfo>::const_iterator it);

	CFontHandle m_font;
	CBrush m_background;
	CCursor m_cursorObj;
	CWindow m_tooltip;

	size_t m_currentZoom;
	std::shared_ptr<ZoomInfo> m_zoom = std::make_shared<ZoomInfo>();
	gui::point m_mouse;
	std::shared_ptr<gui::node> m_hovered;
	std::shared_ptr<gui::node> m_active;
	bool m_tracking = false;
	gui::pointer m_cursor = gui::pointer::arrow;
	Scroller* m_scroller = nullptr;
	std::function<void(const std::wstring&, const std::wstring&)> m_notifier;
	gui::size m_docSize;
	gui::pixels m_fontSize;
	std::string m_fontFamily;
	ani::win32::scene m_scene{TIMER_SCENE};
	std::shared_ptr<ani::animation> m_wait, m_load;

	void updateServers();
	void updateCursor(bool force = false);
	void updateTooltip(bool force = false);
	void updateCursorAndTooltip(bool force = false);
	std::shared_ptr<gui::node> nodeFromPoint();
	std::shared_ptr<gui::node> activeParent(const std::shared_ptr<gui::node>&);
	void setDocumentSize(const gui::size& newSize);
	void updateDocumentSize();

	void mouseFromMessage(LPARAM lParam);

	void zoomIn();
	void zoomOut();
	void setZoom(size_t newLevel);

	void scrollIntoView(const std::shared_ptr<gui::node>& node);
public:
	std::shared_ptr<CAppModel> m_model;
	Strings _;

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
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnMouseDown)
		MESSAGE_HANDLER(WM_LBUTTONUP, OnMouseUp)
		MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
		MESSAGE_HANDLER(UM_LISTCHANGED, OnListChanged)
		MESSAGE_HANDLER(UM_REFRESHSTART, OnRefreshStart)
		MESSAGE_HANDLER(UM_REFRESHSTOP, OnRefreshStop)
		MESSAGE_HANDLER(UM_PROGRESS, OnProgress)
		MESSAGE_HANDLER(UM_LAYOUTNEEDED, OnLayoutNeeded)
		MESSAGE_HANDLER(AM_ZOOM, OnZoom)
		{
			bool handled = false;
			lResult = m_scene.handle_message(uMsg, wParam, lParam, handled);
			if (handled)
				return TRUE;
		}
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
	LRESULT OnLayoutNeeded(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnZoom(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	void setScroller(Scroller* scroller) {
		m_scroller = scroller;
	}

	template <typename T>
	void setNotifier(T&& notifier) {
		m_notifier = notifier;
	}

	bool nextItem();
	bool prevItem();
	void selectItem();
};
