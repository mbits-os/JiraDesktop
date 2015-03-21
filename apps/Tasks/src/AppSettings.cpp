#include "stdafx.h"
#include "AppSettings.h"

namespace servers {
	static jira::server load(const settings::Section& section, const std::string& prefix)
	{
		auto login = section.getString(prefix + "login");
		auto password = section.getBinary(prefix + "password");
		auto url = section.getString(prefix + "url");
		return{ login, password, url, jira::server::stored };
	}

	static void store(const jira::server& server, settings::Section& section, const std::string& prefix)
	{
		section.setString(prefix + "login", server.login());
		section.setBinary(prefix + "password", server.password());
		section.setString(prefix + "url", server.url());
	}

	static std::vector<jira::server> load(const settings::Section& section)
	{
		auto size = section.getUInt32("Items");

		std::vector<jira::server> servers;
		servers.reserve(size);

		for (decltype(size) i = 0; i < size; ++i) {
			servers.push_back(load(section, "Server" + std::to_string(i) + "."));
		}

		return std::move(servers);
	}

	void store(const std::vector<jira::server>& servers, settings::Section& section)
	{
		section.setUInt32("Items", servers.size());

		size_t i = 0;
		for (auto& server : servers) {
			store(server, section, "Server" + std::to_string(i) + ".");
			++i;
		}
	}
}

CAppSettings::CAppSettings() 
	: settings::Section("midnightBITS", "Tasks", "1.0")
{
}

std::vector<jira::server> CAppSettings::servers() const
{
	return servers::load(group("Servers"));
}

void CAppSettings::servers(const std::vector<jira::server>& list)
{
	servers::store(list, group("Servers"));
}
