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
};

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

LRESULT CTasksView::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CPaintDC dc(m_hWnd);

	//TODO: Add your drawing code here

	return 0;
}

LRESULT CTasksView::OnListChanged(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	OutputDebugString(utf::widen(std::to_string((UINT_PTR) m_hWnd) + ": OnListChanged(" +std::to_string(wParam) + ")\n").c_str());

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

	return 0;
}

LRESULT CTasksView::OnRefreshStart(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	OutputDebugString(utf::widen(std::to_string((UINT_PTR) m_hWnd) + ": OnRefreshStart(" + std::to_string(wParam) + ")\n").c_str());
	return 0;
}

LRESULT CTasksView::OnRefreshStop(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	OutputDebugString(utf::widen(std::to_string((UINT_PTR) m_hWnd) + ": OnRefreshStop(" + std::to_string(wParam) + ")\n").c_str());

	auto it = find(wParam);
	if (it == m_servers.end())
		return 0;

	auto& server = *it->m_server;
	it->m_dataset = server.dataset();
	// TODO: update layout
	// TODO: redraw the report table

	auto& dataset = *it->m_dataset;
	std::ostringstream o;
	o << "-----------------------------------------------\n"
		<< "Answer from: " << server.login() << " @ " << server.displayName() << "\n"
		<< "Query: " << (server.view().jql().empty() ? jira::search_def::standard.jql() : server.view().jql()) << "\n";
	OutputDebugString(utf::widen(o.str()).c_str()); o.str("");
	o << "Issues " << (dataset.startAt + 1)
		<< '-' << (dataset.startAt + dataset.data.size())
		<< " of " << dataset.total << ":\n";
	OutputDebugString(utf::widen(o.str()).c_str()); o.str("");

	{
		o << "\n";
		bool first = true;
		for (auto& col : dataset.schema.cols()) {
			if (first) first = false;
			else o << " | ";
			o << col->titleFull();
		}
		o << "\n-----------------------------------------------------------------------\n";
		OutputDebugString(utf::widen(o.str()).c_str()); o.str("");
	}

	for (auto&& row : dataset.data)
		OutputDebugString(utf::widen(row.text(" | ") + "\n").c_str());

	return 0;
}
