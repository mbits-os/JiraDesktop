#include "stdafx.h"
#include "AppSettings.h"

namespace servers {
	static std::shared_ptr<jira::server> load(const settings::Section& section, const std::string& prefix)
	{
		auto name = section.getString(prefix + "name");
		auto login = section.getString(prefix + "login");
		auto password = section.getBinary(prefix + "password");
		auto url = section.getString(prefix + "url");
		auto jql = section.getString(prefix + "query");
		auto fields = section.getString(prefix + "fields");

		return std::make_shared<jira::server>(name, login, password, url, jira::search_def{jql, fields}, jira::server::stored);
	}

	static void store(const jira::server* server, settings::Section& section, const std::string& prefix)
	{
		if (!server->name().empty())
			section.setString(prefix + "name", server->name());
		section.setString(prefix + "login", server->login());
		section.setBinary(prefix + "password", server->password());
		section.setString(prefix + "url", server->url());

		auto& view = server->view();
		if (!view.jql().empty())
			section.setString(prefix + "query", view.jql());
		if (!view.columns().empty())
			section.setString(prefix + "fields", view.columnsDescr());
	}

	static std::vector<std::shared_ptr<jira::server>> load(const settings::Section& section)
	{
		auto size = section.getUInt32("Items");

		std::vector<std::shared_ptr<jira::server>> servers;
		servers.reserve(size);

		for (decltype(size) i = 0; i < size; ++i) {
			servers.push_back(load(section, "Server" + std::to_string(i) + "."));
		}

		return std::move(servers);
	}

	void store(const std::vector<std::shared_ptr<jira::server>>& servers, settings::Section& section)
	{
		section.setUInt32("Items", servers.size());

		size_t i = 0;
		for (auto& server : servers) {
			store(server.get(), section, "Server" + std::to_string(i) + ".");
			++i;
		}
	}
}

CAppSettings::CAppSettings() 
	: settings::Section("midnightBITS", "Tasks", "1.0")
{
}

std::vector<std::shared_ptr<jira::server>> CAppSettings::servers() const
{
	return servers::load(group("Servers"));
}

void CAppSettings::servers(const std::vector<std::shared_ptr<jira::server>>& list)
{
	auto servers = group("Servers");
	servers::store(list, servers);
}
