#include "stdafx.h"
#include "CredentialManager.h"

LRESULT CredentialManager::OnSignalMessage(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	std::string url, realm, user, password;
	{
		// setting up
		std::lock_guard<std::mutex> guard { m_mutex };

		if (m_tasks.empty() || m_showingUI)
			return 0;
		m_showingUI = true;

		auto& task = m_tasks.front();
		ATLASSERT(!task.conns.empty());
		auto& conn = task.conns.front();

		url = task.url;
		realm = task.realm;
		user = conn.owner->get_username();
		password = conn.owner->get_username();
	}

	// show UI
	bool retry = false; // OK was pressed

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
			conn.owner->set_credentials(user, password);
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