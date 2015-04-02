#pragma once

#include "AppModel.h"

enum class Attr {
	Text,
	Href,
	Tooltip
};

class CJiraNode : public IJiraNode, public std::enable_shared_from_this<IJiraNode> {
public:
	std::string text() const override;
	void setTooltip(const std::string& text) override;
	void addChild(const std::shared_ptr<node>& child) override;
	void setClass(jira::styles) override;
	jira::styles getStyles() const override;
	void setClass(rules) override;
	rules getRules() const override;
	const std::vector<std::shared_ptr<node>>& values() const override;

	void paint(IJiraPainter* painter) override;
	void measure(IJiraPainter* painter) override;
	void setPosition(int x, int y) override;
	point getPosition() override;
	point getAbsolutePos() override;
	size getSize() override;

	std::shared_ptr<IJiraNode> getParent() const override;
	void setParent(const std::shared_ptr<IJiraNode>&) override;
	void invalidate() override;
	void invalidate(int x, int y, size_t width, size_t height) override;

	std::shared_ptr<jira::node> nodeFromPoint(int x, int y) override;
	void setHovered(bool hovered) override;
	bool getHovered() const override;
	void setActive(bool active) override;
	bool getActive() const override;
	void activate() override;
	void setCursor(cursor) override;
	cursor getCursor() const override;

	bool hasTooltip() const override;
	const std::string& getTooltip() const override;

	void openLink(const std::string& url);

protected:
	std::map<Attr, std::string> m_data;
	std::vector<std::shared_ptr<jira::node>> m_children;
	jira::styles m_class = jira::styles::unset;
	rules m_rule = rules::body;
	std::weak_ptr<IJiraNode> m_parent;
	struct {
		int x = 0;
		int y = 0;
		size_t width = 0;
		size_t height = 0;
	} m_position;

	std::atomic<int> m_hoverCount{ 0 };
	std::atomic<int> m_activeCount{ 0 };
	cursor m_cursor = cursor::inherited;
};

class CJiraRoot : public CJiraNode {};

class CJiraIconNode : public CJiraNode {
	class ImageCb
		: public ImageRefCallback
		, public std::enable_shared_from_this<ImageCb> {

	public:
		std::weak_ptr<IJiraNode> parent;
		void onImageChange(ImageRef*) override;
	};

	std::shared_ptr<ImageRef> m_image;
	std::shared_ptr<ImageCb> m_cb;
public:
	CJiraIconNode(const std::string& uri, const std::shared_ptr<ImageRef>& image, const std::string& tooltip);
	~CJiraIconNode();
	void attach();
	void addChild(const std::shared_ptr<node>& child) override;
	void paint(IJiraPainter* painter) override;
	void measure(IJiraPainter* painter) override;
};

class CJiraLinkNode : public CJiraNode {
public:
	CJiraLinkNode(const std::string& href);
};

class CJiraTextNode : public CJiraNode {
public:
	CJiraTextNode(const std::string& text);
	void paint(IJiraPainter* painter) override;
	void measure(IJiraPainter* painter) override;
};

class CJiraDocument : public jira::document {
	void setCurrent(const std::shared_ptr<jira::server>&) override;
	std::shared_ptr<jira::node> createTableRow() override;
	std::shared_ptr<jira::node> createEmpty() override;
	std::shared_ptr<jira::node> createSpan() override;
	std::shared_ptr<jira::node> createIcon(const std::string& uri, const std::string& text, const std::string& description) override;
	std::shared_ptr<jira::node> createUser(bool active, const std::string& display, const std::string& email, const std::string& login, std::map<uint32_t, std::string>&& avatar) override;
	std::shared_ptr<jira::node> createLink(const std::string& href) override;
	std::shared_ptr<jira::node> createText(const std::string& text) override;

	std::shared_ptr<ImageRef> createImage(const std::string& uri);

	std::mutex m_guard;
	std::map<std::string, std::shared_ptr<ImageRef>> m_cache;
	std::shared_ptr<jira::server> m_server;
	std::shared_ptr<ImageRef>(*m_creator)(const std::shared_ptr<jira::server>&, const std::string&);

public:
	explicit CJiraDocument(std::shared_ptr<ImageRef>(*creator)(const std::shared_ptr<jira::server>&, const std::string&));
};

class CJiraReportNode : public CJiraNode {
protected:
	CJiraReportNode(const std::shared_ptr<jira::report>& dataset, const std::shared_ptr<std::vector<size_t>>& columns);
	std::weak_ptr<jira::report> m_dataset;
	std::shared_ptr<std::vector<size_t>> m_columns;
public:
	virtual void repositionChildren() = 0;
};

class CJiraRowProxy : public CJiraReportNode {
	size_t m_id;
	std::shared_ptr<IJiraNode> m_proxy;
public:
	CJiraRowProxy(size_t id, const std::shared_ptr<jira::report>& dataset, const std::shared_ptr<std::vector<size_t>>& columns);

	std::string text() const override;
	void setTooltip(const std::string& text) override;
	void addChild(const std::shared_ptr<jira::node>& child) override;
	void setClass(jira::styles) override;
	jira::styles getStyles() const override;
	const std::vector<std::shared_ptr<jira::node>>& values() const override;

	void paint(IJiraPainter* painter) override;
	void measure(IJiraPainter* painter) override;
	void setPosition(int x, int y) override;

	std::shared_ptr<IJiraNode> getParent() const override;
	void setParent(const std::shared_ptr<IJiraNode>&) override;

	void repositionChildren() override;
	std::shared_ptr<jira::node> nodeFromPoint(int x, int y) override;
};

class CJiraHeaderNode : public CJiraReportNode {
public:
	CJiraHeaderNode(const std::shared_ptr<jira::report>& dataset, const std::shared_ptr<std::vector<size_t>>& columns);
	void addChildren();

	void addChild(const std::shared_ptr<jira::node>& child) override final;
	void measure(IJiraPainter* painter) override;
	void repositionChildren() override;
};

class CJiraReportTableNode : public CJiraNode {
	std::shared_ptr<std::vector<size_t>> m_columns;
public:
	explicit CJiraReportTableNode(const std::shared_ptr<jira::report>& dataset);
	void addChildren(const std::shared_ptr<jira::report>& dataset);

	void addChild(const std::shared_ptr<jira::node>& child) override final;
	void measure(IJiraPainter* painter) override;
};

class CJiraReportElement : public CJiraNode {
	std::weak_ptr<jira::report> m_dataset;
	std::function<void(int, int, int, int)> m_invalidator;
public:
	explicit CJiraReportElement(const std::shared_ptr<jira::report>& dataset, const std::function<void(int, int, int, int)>& invalidator);
	void addChildren(const jira::server& server);

	void addChild(const std::shared_ptr<jira::node>& child) override final;
	void measure(IJiraPainter* painter) override;
	void invalidate(int x, int y, size_t width, size_t height) override;
};
