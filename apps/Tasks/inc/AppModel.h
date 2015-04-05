#pragma once

#include <jira\server.hpp>
#include <gui/listeners.hpp>
#include <gui/hotkey.hpp>
#include <gui/image.hpp>
#include <gui/action.hpp>
#include <gui/painter.hpp>

struct CAppModelListener {
	virtual ~CAppModelListener() {}
	virtual void onListChanged(uint32_t addedOrRemoved) = 0;
};

struct StyleSave;

class StyleSaver {
	gui::painter* painter;
	gui::style_handle save;
public:
	explicit StyleSaver(gui::painter* painter, gui::node* node) : painter(painter), save(nullptr)
	{
		if (!node)
			return;

		save = painter->applyStyle(node);
	}

	~StyleSaver()
	{
		painter->restoreStyle(save);
	}
};

class CAppModel : public listeners<CAppModelListener, CAppModel> {
	std::vector<std::shared_ptr<jira::server>> m_servers;
	std::shared_ptr<jira::document> m_document;

	void onListChanged(uint32_t addedOrRemoved);
	std::mutex m_guard;

	static std::shared_ptr<gui::image_ref> image_creator(const std::shared_ptr<jira::server>& srv, const std::string&);
public:
	CAppModel();

	void startup();

	void lock();
	const std::vector<std::shared_ptr<jira::server>>& servers() const;
	const std::shared_ptr<jira::document>& document() const;
	void unlock();

	void add(const std::shared_ptr<jira::server>& server);
	void remove(const std::shared_ptr<jira::server>& server);
	void update(const std::shared_ptr<jira::server>& server);
};
