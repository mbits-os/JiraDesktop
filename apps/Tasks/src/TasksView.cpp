// TasksView.cpp : implementation of the CTasksView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "TasksView.h"
#include <net/utf8.hpp>
#include <algorithm>
#include <sstream>

class TaskViewModelListener : public CAppModelListener {
	HWND m_hWnd;
public:
	TaskViewModelListener(HWND hWnd) : m_hWnd(hWnd) {}

	void onListChanged(uint32_t addedOrRemoved) override
	{
		// traversing the borders of threads, if needed
		// also, the actual reaction to the vent will
		// not block the emit up there
		if (IsWindow(m_hWnd))
			PostMessage(m_hWnd, UM_LISTCHANGED, addedOrRemoved, 0);
	}
};

class ServerListener : public jira::server_listener {
	HWND m_hWnd;
	uint32_t m_sessionId;
public:
	ServerListener(HWND hWnd, uint32_t sessionId) : m_hWnd(hWnd), m_sessionId(sessionId) {}

	void onRefreshStarted() override
	{
		if (IsWindow(m_hWnd))
			PostMessage(m_hWnd, UM_REFRESHSTART, m_sessionId, 0);
	}

	void onRefreshFinished() override
	{
		if (IsWindow(m_hWnd))
			PostMessage(m_hWnd, UM_REFRESHSTOP, m_sessionId, 0);
	}

	void onProgress(bool calculable, uint64_t content, uint64_t loaded) override
	{
		if (!IsWindow(m_hWnd))
			return;

		ProgressInfo info{ content, loaded, calculable };
		SendMessage(m_hWnd, UM_PROGRESS, m_sessionId, (LPARAM)&info);
	}
};

void CTasksView::ServerInfo::calcColumns(CDCHandle dc, CFontHandle text, CFontHandle header)
{
	if (!m_dataset) {
		m_columns.clear();
		return;
	}

	auto older = dc.SelectFont(header);
	SIZE s = {};

	m_columns.resize(m_dataset->schema.cols().size());
	auto dst = m_columns.begin();
	for (auto& col : m_dataset->schema.cols()) {
		auto title = utf::widen(col->titleFull());
		dc.GetTextExtent(title.c_str(), title.length(), &s);
		*dst++ = s.cx;
	}

	dc.SelectFont(text);
	for (auto& item : m_dataset->data) {
		dst = m_columns.begin();
		for (auto& value : item.values()) {
			auto txt = utf::widen(value->text());
			dc.GetTextExtent(txt.c_str(), txt.length(), &s);
			if (s.cx > *dst)
				*dst++ = s.cx;
			else ++dst;
		}
	}
	dc.SelectFont(older);
}

std::vector<CTasksView::ServerInfo>::iterator CTasksView::find(uint32_t sessionId)
{
	return std::find_if(std::begin(m_servers), std::end(m_servers), [sessionId](const ServerInfo& info) { return info.m_sessionId == sessionId; });
}

std::vector<CTasksView::ServerInfo>::iterator CTasksView::insert(std::vector<ServerInfo>::const_iterator it, const std::shared_ptr<jira::server>& server)
{
	// TODO: create UI element
	// TDOD: attach refresh listener to the server
	auto listener = std::make_shared<ServerListener>(m_hWnd, server->sessionId());
	return m_servers.emplace(it, server, listener);
}

std::vector<CTasksView::ServerInfo>::iterator CTasksView::erase(std::vector<ServerInfo>::const_iterator it)
{
	// TODO: destroy UI element
	// TDOD: detach refresh listener from the server
	return m_servers.erase(it);
}


BOOL CTasksView::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}

LRESULT CTasksView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_listener = std::make_shared<TaskViewModelListener>(m_hWnd);
	m_model->registerListener(m_listener);
	return 0;
}

LRESULT CTasksView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_model->unregisterListener(m_listener);
	return 0;
}

class LinePrinter
{
	HFONT older;
	CFontHandle font;
	CDCHandle dc;
	long lineHeight;
	int x = 7;
	int y = 7;

	void updateLineHeight()
	{
		TEXTMETRIC metric = {};
		dc.GetTextMetrics(&metric);
		lineHeight = metric.tmHeight * 12 / 10; // 120%
	}
public:
	explicit LinePrinter(HDC dc_, HFONT font_) : dc(dc_), font(font_)
	{
		older = dc.SelectFont(font);
		updateLineHeight();
	}
	~LinePrinter()
	{
		dc.SelectFont(older);
	}

	LinePrinter& select(HFONT font_)
	{
		font = font_;
		dc.SelectFont(font);
		updateLineHeight();
		return *this;
	}

	LinePrinter& println(const std::wstring& line)
	{
		if (!line.empty())
			dc.TextOut(x, y, line.c_str());
		y += lineHeight;
		x = 7;
		return *this;
	}

	LinePrinter& print(const std::wstring& line)
	{
		if (line.empty())
			return *this;
		SIZE s = {};
		dc.GetTextExtent(line.c_str(), line.length(), &s);
		dc.TextOut(x, y, line.c_str());
		x += s.cx;
		return *this;
	}

	LinePrinter& skipY(double scale)
	{
		y += int(lineHeight * scale);
		x = 7;
		return *this;
	}

	LinePrinter& moveToX(int x_)
	{
		x = x_;
		return *this;
	}

	int getX() const { return x; }
};

LRESULT CTasksView::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CPaintDC dc(m_hWnd);
	dc.SetBkMode(TRANSPARENT);
	LinePrinter out{ (HDC)dc, (HFONT)m_serverHeader };

	for (auto& item : m_servers) {

		std::ostringstream o;

		dc.SetTextColor(0x00883333);

		auto& server = *item.m_server;
		o << server.login() << "@" << server.displayName();
		if (item.m_loading) {
			if (item.m_progress.calculable) {
				o << " " << (100 * item.m_progress.loaded / item.m_progress.content) << "%";
			} else {
				o << " " << item.m_progress.loaded << "B";
			}
		}
		out.skipY(0.25) // margin-top: 0.25em
			.println(utf::widen(o.str()))
			.skipY(0.1); // margin-bottom: 0.1em
		o.str("");

		if (item.m_dataset) {
			dc.SetTextColor(0x00000000);

			auto& dataset = *item.m_dataset;
			if (!dataset.schema.cols().empty()) {
				out.select(m_tableHeader);

				auto x = out.getX();
				auto src = item.m_columns.begin();
				for (auto& col : dataset.schema.cols()) {
					out.moveToX(x).print(utf::widen(col->titleFull()));
					x += *src++ + 14;
				}

				out.println({});
			}

			out.select(m_font);

			for (auto&& row : dataset.data) {
				auto x = out.getX();
				auto src = item.m_columns.begin();
				for (auto& value : row.values()) {
					out.moveToX(x).print(utf::widen(value->text()));
					x += *src++ + 14;
				}
				out.println({});
			}

			o << "(Issues " << (dataset.startAt + 1)
				<< '-' << (dataset.startAt + dataset.data.size())
				<< " of " << dataset.total << ")";
			out.println(utf::widen(o.str()).c_str()); o.str("");
		} else
			out.select(m_font).println(L"Empty");

		out.select(m_serverHeader);
	}

	return 0;
}

void CTasksView::updateLayout()
{
	CWindowDC dc{m_hWnd};

	for (auto& server : m_servers)
		server.calcColumns((HDC)dc, m_font, (HFONT)m_tableHeader);
}

LRESULT CTasksView::OnSetFont(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = FALSE;
	m_font = (HFONT) wParam;

	LOGFONT lf;
	m_font.GetLogFont(&lf);

	// 1em bold
	lf.lfWeight = FW_BOLD;
	m_tableHeader.CreateFontIndirect(&lf);

	// 1.8em
	lf.lfHeight *= 18;
	lf.lfHeight /= 10;
	lf.lfWeight = FW_NORMAL;
	m_serverHeader.CreateFontIndirect(&lf);

	updateLayout();
	// TODO: redraw the report table

	return 0;
}

LRESULT CTasksView::OnListChanged(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// TODO: lock updates
	if (wParam) { 
		// a server has been added or removed...
		auto it = find(wParam);
		if (it == m_servers.end()) { // it was added
			synchronize(*m_model, [&] {
				auto& servers = m_model->servers();
				// find the location of the ID; hint: it might be at the end
				auto rit = std::find_if(std::rbegin(servers), std::rend(servers), [&](const std::shared_ptr<jira::server>& server) { return server->sessionId() == wParam; });
				if (rit != std::rend(servers)) {
					auto it = std::begin(m_servers);

					// it += (rit.base() - servers.begin())
					 std::advance(it, std::distance(std::begin(servers), rit.base()));

					 insert(it, *rit);
				}
			});
		} else
			erase(it);
	} else {
		// no specific item has changed, go over entire model
		synchronize(*m_model, [&] {
			auto& servers = m_model->servers();

			auto it = std::begin(m_servers);
			auto end = std::end(m_servers);

			// for each ServerInfo: if not in model - remove
			while (it != end) {
				auto query = std::find_if(std::begin(servers), std::end(servers), [&](const std::shared_ptr<jira::server>& srv) { return srv->sessionId() == it->m_sessionId; });
				if (query == std::end(servers))
				{
					it = erase(it);
					end = std::end(m_servers);
				}
			}

			// for each server in model: if not in ServerInfos - add
			// side note: we have only items from the model, granted maybe not the ALL items
			// if we kept track of additions/deletions, they are in the same order as in the model
			// we just need to fill in the holes...

			it = std::begin(m_servers);
			end = std::end(m_servers);
			auto srv_it = std::begin(servers);
			auto srv_end = std::end(servers);

			for (; srv_it != srv_end; ++srv_it) {
				if (it != end && (*srv_it)->sessionId() == it->m_sessionId) {
					++it;
					continue;
				}

				it = insert(it, *srv_it); // fill in blanks...
				end = std::end(m_servers);
			}
		});
	}
	// TODO: unlock updates
	Invalidate();

	return 0;
}

LRESULT CTasksView::OnRefreshStart(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	auto it = find(wParam);
	if (it == m_servers.end())
		return 0;

	it->m_progress = ProgressInfo{100, 0, true};
	it->m_loading = true;
	Invalidate();

	return 0;
}

LRESULT CTasksView::OnRefreshStop(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	auto it = find(wParam);
	if (it == m_servers.end())
		return 0;

	it->m_loading = false;

	auto& server = *it->m_server;
	it->m_dataset = server.dataset();
	updateLayout();
	// TODO: redraw the report table
	Invalidate();

	return 0;
}

LRESULT CTasksView::OnProgress(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	if (!lParam)
		return 0;

	auto it = find(wParam);
	if (it == m_servers.end())
		return 0;

	it->m_progress = *reinterpret_cast<ProgressInfo*>(lParam);
	Invalidate();

	return 0;
}