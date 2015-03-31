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

	cast(child)->setParent(this);
	m_children.push_back(std::move(child));
}

void CJiraNode::setClass(jira::styles styles)
{
	m_class = styles;
}

jira::styles CJiraNode::getStyles() const
{
	return m_class;
}

const std::vector<std::unique_ptr<jira::node>>& CJiraNode::values() const
{
	return m_children;
}

void CJiraNode::paint(IJiraPainter* painter)
{
	PushOrigin push{ painter };
	StyleSaver saver{ painter, getStyles() };

	int x = 0;
	// slow, but working:
	for (auto& node : m_children) {
		painter->moveOrigin(x, 0);
		cast(node)->paint(painter);
		x += cast(node)->getSize().width;
	}
}

void CJiraNode::measure(IJiraPainter* painter)
{
	StyleSaver saver{ painter, getStyles() };

	size_t height = 0;
	size_t width = 0;
	int x = 0;
	for (auto& node : m_children) {
		cast(node)->measure(painter);
		auto ret = cast(node)->getSize();
		if (height < ret.height)
			height = ret.height;

		width += ret.width;
		cast(node)->setPosition(x, 0);
		x += ret.height;
	}
	m_position.height = height;
	m_position.width = width;
}

void CJiraNode::setPosition(int x, int y)
{
	m_position.x = x;
	m_position.y = y;
}

IJiraNode::point CJiraNode::getPosition()
{
	return{ m_position.x, m_position.y };
}

IJiraNode::size CJiraNode::getSize()
{
	return{ m_position.width, m_position.height };
}

IJiraNode* CJiraNode::getParent() const
{
	return m_parent;
}

void CJiraNode::setParent(IJiraNode* node)
{
	m_parent = node;
}

void CJiraNode::invalidate()
{
	if (m_parent)
		m_parent->invalidate(); // TODO: only invalidate the rect this node "occupies"
}

CJiraIconNode* parent = nullptr;
void CJiraIconNode::ImageCb::onImageChange(ImageRef*)
{
	parent->invalidate();
}

CJiraIconNode::CJiraIconNode(const std::string& uri, const std::shared_ptr<ImageRef>& image, const std::string& tooltip)
	: m_image(image)
	, m_cb(std::make_shared<ImageCb>())
{
	m_data[Attr::Href] = uri;
	CJiraNode::setTooltip(tooltip);
	m_cb->parent = this;
	m_image->onListenerAdded(m_cb);
	m_position.width = m_position.height = 16;
}

CJiraIconNode::~CJiraIconNode()
{
	m_image->onListenerRemoved(m_cb);
	m_cb->parent = nullptr;
}

void CJiraIconNode::addChild(std::unique_ptr<node>&& /*child*/)
{
	 // noop
}

void CJiraIconNode::paint(IJiraPainter* painter)
{
	painter->paintImage(m_image.get(), 16, 16);
}

void CJiraIconNode::measure(IJiraPainter* /*painter*/)
{
}

CJiraLinkNode::CJiraLinkNode(const std::string& href)
{
	m_data[Attr::Href] = href;
	CJiraNode::setClass(jira::styles::link);
}

CJiraTextNode::CJiraTextNode(const std::string& text)
{
	m_data[Attr::Text] = text;
}

void CJiraTextNode::paint(IJiraPainter* painter)
{
	StyleSaver saver{ painter, getStyles() };

	painter->paintString(m_data[Attr::Text]);
}

void CJiraTextNode::measure(IJiraPainter* painter)
{
	StyleSaver saver{ painter, getStyles() };

	auto size = painter->measureString(m_data[Attr::Text]);
	m_position.width = size.width;
	m_position.height = size.height;
}

CJiraDocument::CJiraDocument(std::shared_ptr<ImageRef>(*creator)(const std::shared_ptr<jira::server>&, const std::string&))
	: m_creator(creator)
{
}

void CJiraDocument::setCurrent(const std::shared_ptr<jira::server>& server)
{
	m_server = server;
}

std::unique_ptr<jira::node> CJiraDocument::createTableRow()
{
	return std::make_unique<CJiraNode>();
}

std::unique_ptr<jira::node> CJiraDocument::createSpan()
{
	return std::make_unique<CJiraNode>();
}

std::unique_ptr<jira::node> CJiraDocument::createIcon(const std::string& uri, const std::string& text, const std::string& description)
{
	auto image = createImage(uri);
	if (text.empty() || description.empty())
		return std::make_unique<CJiraIconNode>(uri, image, text + description);

	return std::make_unique<CJiraIconNode>(uri, image, text + "\n" + description);
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

std::shared_ptr<ImageRef> CJiraDocument::createImage(const std::string& uri)
{
	std::lock_guard<std::mutex> lock(m_guard);
	auto it = m_cache.lower_bound(uri);
	if (it != m_cache.end() && it->first == uri)
		return it->second;

	auto image = m_creator(m_server, uri);
	m_cache.insert(it, std::make_pair(uri, image));
	return image;
}
