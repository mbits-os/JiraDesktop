#pragma once

#include <gui\settings.hpp>
#include <jira\server.hpp>

#include <vector>
#include <memory>

class CAppSettings : public settings::Section {
public:
	CAppSettings();

	std::vector<std::shared_ptr<jira::server>> jiraServers() const;
	void jiraServers(const std::vector<std::shared_ptr<jira::server>>& list);
};

