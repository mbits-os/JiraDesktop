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

struct IJiraNode;
struct IJiraPainter : gui::painter {
	virtual gui::style_handle setStyle(jira::styles, IJiraNode*) = 0;
};

struct IJiraNode : jira::Jnode {
	using point = IJiraPainter::point;
	using size = IJiraPainter::size;

	virtual jira::styles getStyles() const = 0;
};

inline std::shared_ptr<IJiraNode> cast(const std::shared_ptr<jira::node>& node) {
	return std::static_pointer_cast<IJiraNode>(node);
}

class StyleSaver {
	IJiraPainter* painter;
	gui::style_handle save;
public:
	explicit StyleSaver(IJiraPainter* painter, IJiraNode* node) : painter(painter), save(nullptr)
	{
		if (!node)
			return;

		auto style = node->getStyles();
		if (style == jira::styles::unset)
			save = painter->applyStyle(node);
		else
			save = painter->setStyle(style, node);
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
