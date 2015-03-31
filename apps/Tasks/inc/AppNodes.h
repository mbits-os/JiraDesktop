#pragma once

#include "AppModel.h"

enum class Attr {
	Text,
	Href,
	Tooltip
};

class CJiraNode : public IJiraNode {
public:
	std::string text() const override;
	void setTooltip(const std::string& text) override;
	void addChild(std::unique_ptr<node>&& child) override;
	void setClass(jira::styles) override;
	jira::styles getStyles() const override;
	const std::vector<std::unique_ptr<node>>& values() const override;

	void paint(IJiraPainter* painter) override;
	void measure(IJiraPainter* painter) override;
	void setPosition(int x, int y) override;
	point getPosition() override;
	size getSize() override;

	IJiraNode* getParent() const override;
	void setParent(IJiraNode*) override;
	void invalidate() override;

protected:
	std::map<Attr, std::string> m_data;
	std::vector<std::unique_ptr<jira::node>> m_children;
	jira::styles m_class = jira::styles::unset;
	IJiraNode* m_parent = nullptr;
	struct {
		int x = 0;
		int y = 0;
		size_t width = 0;
		size_t height = 0;
	} m_position;
};

class CJiraRoot : public CJiraNode {};

class CJiraIconNode : public CJiraNode {
	class ImageCb
		: public ImageRefCallback
		, public std::enable_shared_from_this<ImageCb> {

	public:
		CJiraIconNode* parent = nullptr;
		void onImageChange(ImageRef*) override;
	};

	std::shared_ptr<ImageRef> m_image;
	std::shared_ptr<ImageCb> m_cb;
public:
	CJiraIconNode(const std::string& uri, const std::shared_ptr<ImageRef>& image, const std::string& tooltip);
	~CJiraIconNode();
	void addChild(std::unique_ptr<node>&& child) override;
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
	std::unique_ptr<jira::node> createTableRow() override;
	std::unique_ptr<jira::node> createSpan() override;
	std::unique_ptr<jira::node> createIcon(const std::string& uri, const std::string& text, const std::string& description) override;
	std::unique_ptr<jira::node> createUser(bool active, const std::string& display, const std::string& email, const std::string& login, std::map<uint32_t, std::string>&& avatar) override;
	std::unique_ptr<jira::node> createLink(const std::string& href) override;
	std::unique_ptr<jira::node> createText(const std::string& text) override;

	std::shared_ptr<ImageRef> createImage(const std::string& uri);

	std::mutex m_guard;
	std::map<std::string, std::shared_ptr<ImageRef>> m_cache;
	std::shared_ptr<jira::server> m_server;
	std::shared_ptr<ImageRef>(*m_creator)(const std::shared_ptr<jira::server>&, const std::string&);

public:
	explicit CJiraDocument(std::shared_ptr<ImageRef>(*creator)(const std::shared_ptr<jira::server>&, const std::string&));
};
