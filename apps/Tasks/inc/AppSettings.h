#pragma once

#include <gui\settings.hpp>
#include <jira\server.hpp>

#include <vector>

class CAppSettings : public settings::Section {
public:
	CAppSettings();

	std::vector<jira::server> servers() const;
	void servers(const std::vector<jira::server>& list);
};

