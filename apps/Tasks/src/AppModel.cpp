#include "stdafx.h"
#include "AppModel.h"
#include "AppSettings.h"
#include <thread>

CAppModel::CAppModel()
{
}

void CAppModel::onListChanged()
{
	emit([](CAppModelListener* listener) { listener->onListChanged(); });
}

void CAppModel::startup()
{
	CAppSettings settings;
	m_servers = settings.servers();

	onListChanged();

	auto local = m_servers;
	for (auto server : local) {
		std::thread{ [server] {
			server->loadFields();
			server->refresh();
		} }.detach();
	}
}

const std::vector<std::shared_ptr<jira::server>>& CAppModel::servers() const
{
	return m_servers;
}

void CAppModel::add(const std::shared_ptr<jira::server>& server)
{
	m_servers.push_back(server);
	onListChanged();

	onUpdate(server);
}

void CAppModel::onUpdate(const std::shared_ptr<jira::server>& server)
{
	CAppSettings settings;
	settings.servers(m_servers);

	server->loadFields();
	server->refresh();
}
