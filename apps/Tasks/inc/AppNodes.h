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
	void paint(IJiraPainter* painter) override;
	std::pair<size_t, size_t> measure(IJiraPainter* painter) override;

protected:
	std::map<Attr, std::string> m_data;
	std::vector<std::unique_ptr<jira::node>> m_children;
	jira::styles m_class = jira::styles::unset;
};

class CJiraIconNode : public CJiraNode {
public:
	CJiraIconNode(const std::string& uri, const std::string& tooltip);
	void addChild(std::unique_ptr<node>&& child) override;
	void paint(IJiraPainter* painter) override;
	std::pair<size_t, size_t> measure(IJiraPainter* painter) override;
};

class CJiraLinkNode : public CJiraNode {
public:
	CJiraLinkNode(const std::string& href);
};

class CJiraTextNode : public CJiraNode {
public:
	CJiraTextNode(const std::string& text);
	void paint(IJiraPainter* painter) override;
	std::pair<size_t, size_t> measure(IJiraPainter* painter) override;
};

class CJiraDocument : public jira::document {
	std::unique_ptr<jira::node> createSpan() override;
	std::unique_ptr<jira::node> createIcon(const std::string& uri, const std::string& text, const std::string& description) override;
	std::unique_ptr<jira::node> createUser(bool active, const std::string& display, const std::string& email, const std::string& login, std::map<uint32_t, std::string>&& avatar) override;
	std::unique_ptr<jira::node> createLink(const std::string& href) override;
	std::unique_ptr<jira::node> createText(const std::string& text) override;
};
