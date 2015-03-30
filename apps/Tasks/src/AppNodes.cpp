#include "stdafx.h"

#ifdef max
#undef max
#endif

#include "AppNodes.h"
#include <limits>

std::string CJiraNode::text() const
{
	auto it = m_data.find(Attr::Text);
	if (it != m_data.end())
		return it->second;

	std::string out;
	for (auto& child : m_children) {
		out += cast(child)->text();
	}
	return out;
}

void CJiraNode::setTooltip(const std::string& text)
{
	m_data[Attr::Tooltip] = text;
}

void CJiraNode::addChild(std::unique_ptr<node>&& child)
{
	auto it = m_data.find(Attr::Text);
	if (it != m_data.end())
		return;

	m_children.push_back(std::move(child));
}

void CJiraNode::paint(IJiraPainter* painter)
{
	PushOrigin push{ painter };

	int x = 0;
	// slow, but working:
	for (auto& node : m_children) {
		painter->moveOrigin(x, 0);
		cast(node)->paint(painter);
		x += std::get<0>(cast(node)->measure(painter));
	}
}

std::pair<size_t, size_t> CJiraNode::measure(IJiraPainter* painter)
{
	size_t height = 0;
	size_t width = 0;
	for (auto& node : m_children) {
		auto ret = cast(node)->measure(painter);
		if (height < std::get<0>(ret))
			height = std::get<0>(ret);

		width += std::get<1>(ret);
	}
	return{ width, height };
}

CJiraIconNode::CJiraIconNode(const std::string& uri, const std::string& tooltip)
{
	m_data[Attr::Href] = uri;
	CJiraNode::setTooltip(tooltip);
}

void CJiraIconNode::addChild(std::unique_ptr<node>&& /*child*/)
{
	 // noop
}

void CJiraIconNode::paint(IJiraPainter* painter)
{
	painter->paintImage(m_data[Attr::Href], 16, 16);
}

std::pair<size_t, size_t> CJiraIconNode::measure(IJiraPainter* /*painter*/)
{
	return{ 16, 16 };
}

CJiraLinkNode::CJiraLinkNode(const std::string& href)
{
	m_data[Attr::Href] = href;
}

void CJiraLinkNode::paint(IJiraPainter* painter)
{
	// set color from style
	CJiraNode::paint(painter);
	// restore color
}

CJiraTextNode::CJiraTextNode(const std::string& text)
{
	m_data[Attr::Text] = text;
}

void CJiraTextNode::paint(IJiraPainter* painter)
{
	painter->paintString(m_data[Attr::Text]);
}

std::pair<size_t, size_t> CJiraTextNode::measure(IJiraPainter* painter)
{
	return painter->measureString(m_data[Attr::Text]);
}

std::unique_ptr<jira::node> CJiraDocument::createSpan()
{
	return std::make_unique<CJiraNode>();
}

std::unique_ptr<jira::node> CJiraDocument::createIcon(const std::string& uri, const std::string& text, const std::string& description)
{
	if (text.empty() || description.empty())
		return std::make_unique<CJiraIconNode>(uri, text + description);

	return std::make_unique<CJiraIconNode>(uri, text + "\n" + description);
}

std::unique_ptr<jira::node> CJiraDocument::createUser(bool /*active*/, const std::string& display, const std::string& email, const std::string& /*login*/, std::map<uint32_t, std::string>&& avatar)
{
	auto av = std::move(avatar);
	constexpr size_t defSize = 16;
	std::string image;
	auto it = av.find(defSize);
	if (it != av.end())
		image = it->second;
	else {
		size_t delta = std::numeric_limits<size_t>::max();
		for (auto& pair : av) {
			size_t d = pair.first > defSize ? pair.first - defSize : defSize - pair.first;
			if (d < delta) {
				delta = d;
				image = pair.second;
			}
		}
	}

	if (image.empty()) {
		auto node = createText(display);
		if (!email.empty())
			node->setTooltip(email);
		return std::move(node);
	}

	return createIcon(image, display, email);
}

std::unique_ptr<jira::node> CJiraDocument::createLink(const std::string& href)
{
	return std::make_unique<CJiraLinkNode>(href);
}

std::unique_ptr<jira::node> CJiraDocument::createText(const std::string& text)
{
	return std::make_unique<CJiraTextNode>(text);
}