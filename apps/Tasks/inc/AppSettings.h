#pragma once

#include <gui\settings.hpp>
#include <jira\server.hpp>

#include <vector>
#include <memory>

class CAppSettings : public settings::Section {
public:
	CAppSettings();

	std::vector<std::shared_ptr<jira::server>> jiraServers();
	void jiraServers(const std::vector<std::shared_ptr<jira::server>>& list);
	std::string language();
	void language(const std::string& lng);
	std::string lastVersion();
	void lastVersion(const std::string& ver);
};

