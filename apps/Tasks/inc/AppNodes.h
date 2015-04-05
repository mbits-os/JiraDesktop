#pragma once

#include "AppModel.h"

enum class Attr {
	Text,
	Href,
	Tooltip
};

class CJiraNode : public gui::node, public std::enable_shared_from_this<gui::node> {
public:
	CJiraNode(gui::elem name);
	gui::elem getNodeName() const override;
	void addClass(const std::string& name) override;
	void removeClass(const std::string& name) override;
	bool hasClass(const std::string& name) const override;
	std::string text() const override;
	void setTooltip(const std::string& text) override;
	void addChild(const std::shared_ptr<node>& child) override;
	const std::vector<std::shared_ptr<node>>& children() const override;

	void paint(gui::painter* painter) override;
	void measure(gui::painter* painter) override;
	void setPosition(int x, int y) override;
	point getPosition() override;
	point getAbsolutePos() override;
	size getSize() override;

	std::shared_ptr<gui::node> getParent() const override;
	void setParent(const std::shared_ptr<gui::node>&) override;
	void invalidate() override;
	void invalidate(int x, int y, size_t width, size_t height) override;

	std::shared_ptr<gui::node> nodeFromPoint(int x, int y) override;
	void setHovered(bool hovered) override;
	bool getHovered() const override;
	void setActive(bool active) override;
	bool getActive() const override;
	void activate() override;
	void setCursor(gui::cursor) override;
	gui::cursor getCursor() const override;

	bool hasTooltip() const override;
	const std::string& getTooltip() const override;

	void openLink(const std::string& url);

protected:
	gui::elem m_nodeName;
	std::map<Attr, std::string> m_data;
	std::vector<std::shared_ptr<node>> m_children;
	std::vector<std::string> m_classes;
	std::weak_ptr<node> m_parent;
	struct {
		int x = 0;
		int y = 0;
		size_t width = 0;
		size_t height = 0;
	} m_position;

	std::atomic<int> m_hoverCount{ 0 };
	std::atomic<int> m_activeCount{ 0 };
	gui::cursor m_cursor = gui::cursor::inherited;
};

class ImageCb
	: public gui::image_ref_callback
	, public std::enable_shared_from_this<ImageCb> {

public:
	std::weak_ptr<gui::node> parent;
	void onImageChange(gui::image_ref*) override;
};

class CJiraIconNode : public CJiraNode {
	std::shared_ptr<gui::image_ref> m_image;
	std::shared_ptr<ImageCb> m_cb;
public:
	CJiraIconNode(const std::string& uri, const std::shared_ptr<gui::image_ref>& image, const std::string& tooltip);
	~CJiraIconNode();
	void attach();
	void addChild(const std::shared_ptr<node>& child) override;
	void paint(gui::painter* painter) override;
	void measure(gui::painter* painter) override;
};

class CJiraDocument;
class CJiraUserNode : public CJiraNode {
	std::weak_ptr<CJiraDocument> m_document;
	std::shared_ptr<gui::image_ref> m_image;
	std::shared_ptr<ImageCb> m_cb;
	std::map<uint32_t, std::string> m_urls;
	int m_selectedSize;
public:
	CJiraUserNode(const std::weak_ptr<CJiraDocument>& document, std::map<uint32_t, std::string>&& avatar, const std::string& tooltip);
	~CJiraUserNode();
	void addChild(const std::shared_ptr<node>& child) override;
	void paint(gui::painter* painter) override;
	void measure(gui::painter* painter) override;
};

class CJiraLinkNode : public CJiraNode {
public:
	CJiraLinkNode(const std::string& href);
};

class CJiraTextNode : public CJiraNode {
public:
	CJiraTextNode(const std::string& text);
	void paint(gui::painter* painter) override;
	void measure(gui::painter* painter) override;
};

class CJiraDocument : public jira::document, public std::enable_shared_from_this<CJiraDocument> {
	void setCurrent(const std::shared_ptr<jira::server>&) override;
	std::shared_ptr<gui::node> createTableRow() override;
	std::shared_ptr<gui::node> createEmpty() override;
	std::shared_ptr<gui::node> createSpan() override;
	std::shared_ptr<gui::node> createIcon(const std::string& uri, const std::string& text, const std::string& description) override;
	std::shared_ptr<gui::node> createUser(bool active, const std::string& display, const std::string& email, const std::string& login, std::map<uint32_t, std::string>&& avatar) override;
	std::shared_ptr<gui::node> createLink(const std::string& href) override;
	std::shared_ptr<gui::node> createText(const std::string& text) override;

	std::mutex m_guard;
	std::map<std::string, std::shared_ptr<gui::image_ref>> m_cache;
	std::shared_ptr<jira::server> m_server;
	std::shared_ptr<gui::image_ref>(*m_creator)(const std::shared_ptr<jira::server>&, const std::string&);

public:
	explicit CJiraDocument(std::shared_ptr<gui::image_ref>(*creator)(const std::shared_ptr<jira::server>&, const std::string&));

	std::shared_ptr<gui::image_ref> createImage(const std::string& uri);
};

class CJiraTableNode : public CJiraNode {
	std::shared_ptr<std::vector<size_t>> m_columns;
public:
	CJiraTableNode();
	std::shared_ptr<node> addHeader();
	std::shared_ptr<node> addRow();

	void addChild(const std::shared_ptr<node>& child) override final;
	void measure(gui::painter* painter) override;
};

class CJiraTableRowNode : public CJiraNode {
protected:
	std::shared_ptr<std::vector<size_t>> m_columns;
public:
	CJiraTableRowNode(gui::elem name, const std::shared_ptr<std::vector<size_t>>& columns);

	void measure(gui::painter* painter) override;

	void repositionChildren();
};


class CJiraReportNode : public CJiraNode {
protected:
	CJiraReportNode(gui::elem name, const std::shared_ptr<jira::report>& dataset, const std::shared_ptr<std::vector<size_t>>& columns);
	std::weak_ptr<jira::report> m_dataset;
	std::shared_ptr<std::vector<size_t>> m_columns;
public:
	virtual void repositionChildren() = 0;
};

class CJiraRowProxy : public CJiraReportNode {
	size_t m_id;
	std::shared_ptr<gui::node> m_proxy;
public:
	CJiraRowProxy(size_t id, const std::shared_ptr<jira::report>& dataset, const std::shared_ptr<std::vector<size_t>>& columns);

	std::string text() const override;
	void addClass(const std::string& name) override;
	void removeClass(const std::string& name) override;
	bool hasClass(const std::string& name) const override;
	void setTooltip(const std::string& text) override;
	void addChild(const std::shared_ptr<node>& child) override;
	const std::vector<std::shared_ptr<node>>& children() const override;

	void paint(gui::painter* painter) override;
	void measure(gui::painter* painter) override;
	void setPosition(int x, int y) override;

	std::shared_ptr<node> getParent() const override;
	void setParent(const std::shared_ptr<node>&) override;

	void repositionChildren() override;
	std::shared_ptr<node> nodeFromPoint(int x, int y) override;
};

class CJiraHeaderNode : public CJiraReportNode {
public:
	CJiraHeaderNode(const std::shared_ptr<jira::report>& dataset, const std::shared_ptr<std::vector<size_t>>& columns);
	void addChildren();

	void addChild(const std::shared_ptr<node>& child) override final;
	void measure(gui::painter* painter) override;
	void repositionChildren() override;
};

class CJiraReportTableNode : public CJiraNode {
	std::shared_ptr<std::vector<size_t>> m_columns;
public:
	explicit CJiraReportTableNode(const std::shared_ptr<jira::report>& dataset);
	void addChildren(const std::shared_ptr<jira::report>& dataset);

	void addChild(const std::shared_ptr<node>& child) override final;
	void measure(gui::painter* painter) override;
};

class CJiraReportElement : public CJiraNode {
	std::weak_ptr<jira::report> m_dataset;
	std::function<void(int, int, int, int)> m_invalidator;
public:
	explicit CJiraReportElement(const std::shared_ptr<jira::report>& dataset, const std::function<void(int, int, int, int)>& invalidator);
	void addChildren(const jira::server& server);

	void addChild(const std::shared_ptr<node>& child) override final;
	void measure(gui::painter* painter) override;
	void invalidate(int x, int y, size_t width, size_t height) override;
};
