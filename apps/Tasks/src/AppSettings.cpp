#include "stdafx.h"
#include "AppSettings.h"
#include "version.h"

using namespace std::literals;

namespace servers {
	static std::shared_ptr<jira::server> load_srv(const settings::Section& section)
	{
		auto name = section.getString("Name");
		auto login = section.getString("Login");
		auto password = section.getBinary("Password");
		auto url = section.getString("URL");

		auto items = section.getUInt32("Items");
		std::vector<jira::search_def> views;
		views.reserve(items);

		for (size_t id = 0; id < items; ++id) {
			auto sub = section.group(std::to_string(id));
			auto title = sub.getString("Title");
			auto jql = sub.getString("Query");
			auto fields = sub.getString("Fields");
			auto timeout = sub.contains("Timeout") ?
				std::chrono::seconds{ sub.getUInt32("Timeout") } : std::chrono::milliseconds::max();

			views.emplace_back(title, jql, fields, timeout);
		}

		return std::make_shared<jira::server>(name, login, password, url, views, jira::server::stored);
	}

	static void store_srv(const jira::server* server, settings::Section& section)
	{
		if (!server->name().empty())
			section.setString("Name", server->name());
		else
			section.unset("Name");

		section.setString("Login", server->login());
		section.setBinary("Password", server->password());
		section.setString("URL", server->url());

		auto& views = server->views();
		section.setUInt32("Items", views.size());

		size_t id = 0;
		for (auto& view : views) {
			auto sub = section.group(std::to_string(id++));
			sub.setString("Type", "query");
			if (!view.jql().empty()) {
				sub.setString("Query", view.jql());
				if (!view.title().empty() && view.jql() != view.title())
					sub.setString("Title", view.title());
				else
					sub.unset("Title");
			}
			else {
				sub.unset("Query");
				sub.unset("Title");
			}

			if (!view.columns().empty())
				sub.setString("Fields", view.columnsDescr());
			else
				sub.unset("Fields");

			if (view.timeout().count() < std::numeric_limits<uint32_t>::max())
				sub.setUInt32("Timeout", (uint32_t)view.timeout().count());
			else
				sub.unset("Timeout");
		}
	}

	static std::vector<std::shared_ptr<jira::server>> load(const settings::Section& section)
	{
		auto size = section.getUInt32("Items");

		std::vector<std::shared_ptr<jira::server>> servers;
		servers.reserve(size);

		for (decltype(size) i = 0; i < size; ++i) {
			auto sub = section.group(std::to_string(i));
			servers.push_back(load_srv(sub));
		}

		return std::move(servers);
	}

	void store(const std::vector<std::shared_ptr<jira::server>>& servers, settings::Section& section)
	{
		section.setUInt32("Items", servers.size());

		size_t i = 0;
		for (auto& server : servers) {
			auto sub = section.group(std::to_string(i));
			store_srv(server.get(), sub);
			++i;
		}
	}

	namespace zero {
		static std::shared_ptr<jira::server> load(const settings::Section& section, const std::string& prefix)
		{
			auto name = section.getString(prefix + "name");
			auto login = section.getString(prefix + "login");
			auto password = section.getBinary(prefix + "password");
			auto url = section.getString(prefix + "url");
			auto jql = section.getString(prefix + "query");
			auto fields = section.getString(prefix + "fields");
			auto timeout = section.contains(prefix + "timeout") ?
				std::chrono::seconds{ section.getUInt32(prefix + "timeout") } : std::chrono::milliseconds::max();

			std::vector<jira::search_def> views;
			views.emplace_back(""s, jql, fields, timeout);
			return std::make_shared<jira::server>(name, login, password, url, views, jira::server::stored);
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

std::vector<std::shared_ptr<jira::server>> CAppSettings::jiraServers()
{
	if (!group("Jira"s).hasGroup("Servers"s) && hasGroup("JiraServers"s)) {
		auto servers = group("Jira"s).group("Servers"s);
		servers::store(servers::zero::load(group("JiraServers"s)), servers);
		unsetGroup("JiraServers"s);
	}

	return servers::load(group("Jira"s).group("Servers"s));
}

void CAppSettings::jiraServers(const std::vector<std::shared_ptr<jira::server>>& list)
{
	group("Jira"s).unsetGroup("Servers"s);
	auto servers = group("Jira"s).group("Servers"s);
	servers::store(list, servers);
}
