#include "stdafx.h"
#include "AppSettings.h"
#include "version.h"

namespace servers {
	static std::shared_ptr<jira::server> load(const settings::Section& section, const std::string& prefix)
	{
		auto name = section.getString(prefix + "name");
		auto login = section.getString(prefix + "login");
		auto password = section.getBinary(prefix + "password");
		auto url = section.getString(prefix + "url");
		auto jql = section.getString(prefix + "query");
		auto fields = section.getString(prefix + "fields");
		auto timeout = section.getType(prefix + "timeout") == settings::UInt32 ?
			std::chrono::seconds{ section.getUInt32(prefix + "timeout") } : std::chrono::milliseconds::max();

		return std::make_shared<jira::server>(name, login, password, url, jira::search_def{ jql, fields, timeout }, jira::server::stored);
	}

	static void store(const jira::server* server, settings::Section& section, const std::string& prefix)
	{
		if (!server->name().empty())
			section.setString(prefix + "name", server->name());
		else
			section.unset(prefix + "name");

		section.setString(prefix + "login", server->login());
		section.setBinary(prefix + "password", server->password());
		section.setString(prefix + "url", server->url());

		auto& view = server->view();
		if (!view.jql().empty())
			section.setString(prefix + "query", view.jql());
		else
			section.unset(prefix + "query");

		if (!view.columns().empty())
			section.setString(prefix + "fields", view.columnsDescr());
		else
			section.unset(prefix + "field");

		if (view.timeout().count() < std::numeric_limits<uint32_t>::max())
			section.setUInt32(prefix + "timeout", (uint32_t)view.timeout().count());
		else
			section.unset(prefix + "timeout");
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

#if PROGRAM_VERSION_MAJOR == 0
#define SETTINGS_VERSION "1.0"
#else
#define SETTINGS_VERSION VERSION_STRINGIFY(PROGRAM_VERSION_MAJOR) ".0"
#endif

CAppSettings::CAppSettings() 
	: settings::Section(PROGRAM_COPYRIGHT_HOLDER, PROGRAM_NAME, SETTINGS_VERSION)
{
}

std::vector<std::shared_ptr<jira::server>> CAppSettings::jiraServers() const
{
	return servers::load(group("JiraServers"));
}

void CAppSettings::jiraServers(const std::vector<std::shared_ptr<jira::server>>& list)
{
	unsetGroup("Servers");
	auto servers = group("JiraServers");
	servers::store(list, servers);
}
