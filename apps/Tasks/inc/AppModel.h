#pragma once

#include <jira\server.hpp>
#include <jira\listeners.hpp>

struct CAppModelListener {
	virtual ~CAppModelListener() {}
	virtual void onListChanged() = 0;
};

class CAppModel : jira::listeners<CAppModelListener> {
	std::vector<std::shared_ptr<jira::server>> m_servers;

	void onListChanged();
public:
	CAppModel();

	void startup();

	const std::vector<std::shared_ptr<jira::server>>& servers() const;
	void add(const std::shared_ptr<jira::server>& server);
	void onUpdate(const std::shared_ptr<jira::server>& server);
};

