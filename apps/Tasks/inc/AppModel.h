#pragma once

#include <jira\server.hpp>
#include <gui/listeners.hpp>
#include <gui/hotkey.hpp>
#include <gui/image.hpp>
#include <gui/action.hpp>
#include <gui/painter.hpp>
#include <gui/nodes/block_node.hpp>

struct CAppModelListener {
	virtual ~CAppModelListener() {}
	virtual void onListChanged(uint32_t addedOrRemoved) = 0;
};

struct ServerInfo {
	std::shared_ptr<jira::server> m_server;
	std::shared_ptr<gui::document> m_document;
};

class CAppModel : public listeners<CAppModelListener, CAppModel> {
	std::vector<ServerInfo> m_servers;

	void onListChanged(uint32_t addedOrRemoved);
	std::mutex m_guard;
public:
	CAppModel();

	void startup();

	void lock();
	const std::vector<ServerInfo>& servers() const;
	void unlock();

	void add(const std::shared_ptr<jira::server>& server);
	void remove(const std::shared_ptr<jira::server>& server);
	void update(const std::shared_ptr<jira::server>& server);
};
