#pragma once

#include "AppModel.h"
#include <gui/styles.hpp>

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
	const std::vector<std::string>& getClassNames() const override;
	std::string text() const override;
	void setTooltip(const std::string& text) override;
	void addChild(const std::shared_ptr<node>& child) override;
	const std::vector<std::shared_ptr<node>>& children() const override;

	void paint(gui::painter* painter) override final;
	void measure(gui::painter* painter) override final;
	void setPosition(const gui::pixels& x, const gui::pixels& y) override;
	void setSize(const gui::pixels& width, const gui::pixels& height) override;
	gui::point getPosition() override;
	gui::point getAbsolutePos() override;
	gui::size getSize() override;

	std::shared_ptr<gui::node> getParent() const override;
	void setParent(const std::shared_ptr<gui::node>&) override;
	void invalidate() override;
	void invalidate(const gui::point& pt, const gui::size& size) override;

	std::shared_ptr<gui::node> nodeFromPoint(const gui::pixels& x, const gui::pixels& y) override;
	void setHovered(bool hovered) override;
	bool getHovered() const override;
	void setActive(bool active) override;
	bool getActive() const override;
	void activate() override;
	gui::pointer getCursor() const override;

	bool hasTooltip() const override;
	const std::string& getTooltip() const override;

	void innerText(const std::string& text) override;

	std::shared_ptr<styles::rule_storage> calculatedStyle() const override;
	std::shared_ptr<styles::rule_storage> normalCalculatedStyles() const override;
	std::shared_ptr<styles::stylesheet> styles() const override;
	void applyStyles(const std::shared_ptr<styles::stylesheet>& stylesheet) override;
	void calculateStyles();
	gui::pixels offsetLeft() const; // border-left-width + padding-left
	gui::pixels offsetTop() const; // border-top-width + padding-top
	gui::pixels offsetRight() const; // border-right-width + padding-right
	gui::pixels offsetBottom() const; // border-bottom-width + padding-bottom

	void openLink(const std::string& url);
	virtual void paintContents(gui::painter* painter,
		const gui::pixels& offX, const gui::pixels& offY);
	virtual gui::size measureContents(gui::painter* painter,
		const gui::pixels& offX, const gui::pixels& offY) = 0;

protected:
	gui::elem m_nodeName;
	std::map<Attr, std::string> m_data;
	std::vector<std::shared_ptr<node>> m_children;
	std::vector<std::string> m_classes;
	std::weak_ptr<node> m_parent;
	struct {
		gui::point pt;
		gui::size size;
	} m_position;

	std::atomic<int> m_hoverCount{ 0 };
	std::atomic<int> m_activeCount{ 0 };

	std::shared_ptr<styles::rule_storage> m_calculated;
	std::shared_ptr<styles::rule_storage> m_calculatedHover;
	std::shared_ptr<styles::rule_storage> m_calculatedActive;
	std::shared_ptr<styles::rule_storage> m_calculatedHoverActive;
	std::shared_ptr<styles::stylesheet> m_allApplying;
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
	void paintContents(gui::painter* painter,
		const gui::pixels& offX, const gui::pixels& offY) override;
	gui::size measureContents(gui::painter* painter,
		const gui::pixels& offX, const gui::pixels& offY) override;
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
	void paintContents(gui::painter* painter,
		const gui::pixels& offX, const gui::pixels& offY) override;
	gui::size measureContents(gui::painter* painter,
		const gui::pixels& offX, const gui::pixels& offY) override;
};

class CJiraBlockNode : public CJiraNode {
public:
	CJiraBlockNode(gui::elem name);
	gui::size measureContents(gui::painter* painter,
		const gui::pixels& offX, const gui::pixels& offY) override;
};

class CJiraSpanNode : public CJiraNode {
public:
	CJiraSpanNode(gui::elem name);
	gui::size measureContents(gui::painter* painter,
		const gui::pixels& offX, const gui::pixels& offY) override;
};

class CJiraLinkNode : public CJiraSpanNode {
public:
	CJiraLinkNode(const std::string& href);
};

class CJiraTextNode : public CJiraNode {
public:
	CJiraTextNode(const std::string& text);
	void paintContents(gui::painter* painter,
		const gui::pixels& offX, const gui::pixels& offY) override;
	gui::size measureContents(gui::painter* painter,
		const gui::pixels& offX, const gui::pixels& offY) override;
};

class CJiraDocument : public jira::document, public std::enable_shared_from_this<CJiraDocument> {
	void setCurrent(const std::shared_ptr<jira::server>&) override;
	std::shared_ptr<gui::node> createIcon(const std::string& uri, const std::string& text, const std::string& description) override;
	std::shared_ptr<gui::node> createUser(bool active, const std::string& display, const std::string& email, const std::string& login, std::map<uint32_t, std::string>&& avatar) override;
	std::shared_ptr<gui::node> createLink(const std::string& href) override;
	std::shared_ptr<gui::node> createText(const std::string& text) override;
	std::shared_ptr<gui::node> createElement(const gui::elem name) override;

	std::mutex m_guard;
	std::map<std::string, std::shared_ptr<gui::image_ref>> m_cache;
	std::shared_ptr<jira::server> m_server;
	std::shared_ptr<gui::image_ref>(*m_creator)(const std::shared_ptr<jira::server>&, const std::string&);

public:
	explicit CJiraDocument(std::shared_ptr<gui::image_ref>(*creator)(const std::shared_ptr<jira::server>&, const std::string&));

	std::shared_ptr<gui::image_ref> createImage(const std::string& uri);
};

class CJiraTableNode : public CJiraBlockNode {
	std::shared_ptr<std::vector<gui::pixels>> m_columns;
public:
	CJiraTableNode();

	void addChild(const std::shared_ptr<node>& child) override final;
	gui::size measureContents(gui::painter* painter,
		const gui::pixels& offX, const gui::pixels& offY) override;
};

class CJiraTableRowNode : public CJiraNode {
protected:
	std::shared_ptr<std::vector<gui::pixels>> m_columns;
public:
	CJiraTableRowNode(gui::elem name);
	void setColumns(const std::shared_ptr<std::vector<gui::pixels>>& columns);

	gui::size measureContents(gui::painter* painter,
		const gui::pixels& offX, const gui::pixels& offY) override;

	void repositionChildren(gui::painter* painter);
};

class CJiraReportElement : public CJiraBlockNode {
	std::weak_ptr<jira::report> m_dataset;
public:
	explicit CJiraReportElement(const std::shared_ptr<jira::report>& dataset);
	void addChildren(const jira::server& server);

	void addChild(const std::shared_ptr<node>& child) override final;
};

class CJiraDocumentElement : public CJiraBlockNode {
	std::function<void(const gui::point&, const gui::size&)> m_invalidator;
public:
	explicit CJiraDocumentElement(const std::function<void(const gui::point&, const gui::size&)>& invalidator);
	void invalidate(const gui::point& pt, const gui::size& size) override;
};
