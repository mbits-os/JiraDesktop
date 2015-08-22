#include "stdafx.h"
#include "resource.h"
#include "CredentialManager.h"
#include "LoginDlg.h"

LRESULT CredentialManager::OnSignalMessage(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CLoginDlg dlg;
	{
		// setting up
		std::lock_guard<std::mutex> guard { m_mutex };

		if (m_tasks.empty() || m_showingUI)
			return 0;
		m_showingUI = true;

		auto& task = m_tasks.front();
		ATLASSERT(!task.conns.empty());
		auto& conn = task.conns.front();

		dlg.serverUrl = task.url;
		dlg.serverRealm = task.realm;
		dlg.userName = conn.owner->get_username();
		dlg.userPassword = conn.owner->get_username();
	}

	// show UI
	bool retry = dlg.DoModal(m_hWnd) == IDOK;

	Task task;
	{
		std::lock_guard<std::mutex> guard { m_mutex };

		m_showingUI = false;
		task = std::move(m_tasks.front());
		m_tasks.erase(m_tasks.begin());

		if (!m_tasks.empty())
			PostMessage(m_hWnd, pingMsg(), 0, 0);
	}

	if (retry) {
		for (auto& conn : task.conns)
			conn.owner->set_credentials(dlg.userName, dlg.userPassword);
		// TODO: trigger settings update
	}

	for (auto& conn : task.conns)
		conn.promise.set_value(retry);

	return 0;
}

std::future<bool> CredentialManager::attach(const std::shared_ptr<owner>& owner, const std::string& url, const std::string& realm)
{
	std::lock_guard<std::mutex> guard { m_mutex };

	TaskConn conn;
	conn.owner = owner;
	auto future = conn.promise.get_future();

	auto key = owner->key();
	auto it = std::find_if(std::begin(m_tasks), std::end(m_tasks), [&](auto& item) { return item.key == key; });

	if (it == std::end(m_tasks)) {
		Task newTask { key, url, realm };
		newTask.conns.emplace_back(std::move(conn));
		m_tasks.emplace_back(std::move(newTask));
	} else {
		it->conns.emplace_back(std::move(conn));
	}

	return future;
}


std::future<bool> CredentialManager::ask_user(const std::shared_ptr<owner>& owner, const std::string& url, const std::string& realm)
{
	if (!owner) {
		std::promise<bool> preset;
		auto future = preset.get_future();
		preset.set_value(false);
		return future;
	}

	auto future = attach(owner, url, realm);

	ATLASSERT(m_boundThread != GetCurrentThreadId());
	PostMessage(m_hWnd, pingMsg(), 0, 0);

	return future;
}