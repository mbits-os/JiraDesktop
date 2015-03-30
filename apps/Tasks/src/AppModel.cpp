#include "stdafx.h"
#include "AppModel.h"
#include "AppNodes.h"
#include "AppSettings.h"
#include <thread>

CAppModel::CAppModel()
	: m_document(std::make_shared<CJiraDocument>())
{
}

void CAppModel::onListChanged(uint32_t addedOrRemoved)
{
	emit([addedOrRemoved](CAppModelListener* listener) { listener->onListChanged(addedOrRemoved); });
}

void CAppModel::startup()
{
	synchronize(m_guard, [&] {
		CAppSettings settings;
		m_servers = settings.jiraServers();
	});

	onListChanged(0);

	auto local = m_servers;
	auto document = m_document;
	for (auto server : local) {
		std::thread{ [server, document] {
			server->loadFields();
			server->refresh(document);
		} }.detach();
	}
}

void CAppModel::lock()
{
	m_guard.lock();
}

void CAppModel::unlock()
{
	m_guard.unlock();
}

const std::vector<std::shared_ptr<jira::server>>& CAppModel::servers() const
{
	return m_servers;
}

const std::shared_ptr<jira::document>& CAppModel::document() const
{
	return m_document;
}

void CAppModel::add(const std::shared_ptr<jira::server>& server)
{
	if (!server)
		return;

	synchronize(m_guard, [&]{
		m_servers.push_back(server);
	});
	onListChanged(server->sessionId());

	update(server);
}

void CAppModel::remove(const std::shared_ptr<jira::server>& server)
{
	auto id = server ? server->sessionId() : 0;
	if (server) {
		synchronize(m_guard, [&] {
			auto it = m_servers.begin();
			auto end = m_servers.end();
			for (; it != end; ++it) {
				auto& server = *it;
				if (server && server->sessionId() == id) {
					m_servers.erase(it);
					break;
				}
			}

			CAppSettings settings;
			settings.jiraServers(m_servers);
		});
	}

	onListChanged(id);
}

void CAppModel::update(const std::shared_ptr<jira::server>& server)
{
	if (!server)
		return;

	synchronize(m_guard, [&] {
		CAppSettings settings;
		settings.jiraServers(m_servers);
	});

	auto document = m_document;
	std::thread{ [server, document] {
		server->loadFields();
		server->refresh(document);
	} }.detach();
}
