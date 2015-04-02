#include "stdafx.h"

#ifdef max
#undef max
#endif

#include "AppNodes.h"
#include <limits>
#include <sstream>

enum {
	CELL_MARGIN = 7
};

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
	m_rule = rules::body;
}

jira::styles CJiraNode::getStyles() const
{
	return m_class;
}

void CJiraNode::setClass(rules rule)
{
	m_rule = rule;
	m_class = jira::styles::unset;
}

rules CJiraNode::getRules() const
{
	return m_rule;
}

const std::vector<std::unique_ptr<jira::node>>& CJiraNode::values() const
{
	return m_children;
}

void CJiraNode::paint(IJiraPainter* painter)
{
	StyleSaver saver{ painter, this };

	// slow, but working:
	for (auto& node : m_children) {
		PushOrigin push{ painter };
		painter->moveOrigin(cast(node)->getPosition());
		cast(node)->paint(painter);
	}
}

void CJiraNode::measure(IJiraPainter* painter)
{
	StyleSaver saver{ painter, this };

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
		x += ret.width;
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
	invalidate(0, 0, m_position.width, m_position.height);
}

void CJiraNode::invalidate(int x, int y, size_t width, size_t height)
{
	x += m_position.x;
	y += m_position.y;
	if (m_parent)
		m_parent->invalidate(x, y, width, height);
}

jira::node* CJiraNode::findHovered(int x, int y)
{
	x -= m_position.x;
	y -= m_position.y;

	if (x < 0 || (size_t)x > m_position.width ||
		y < 0 || (size_t)y > m_position.height)
		return nullptr;

	for (auto& node : m_children) {
		auto tmp = cast(node)->findHovered(x, y);
		if (tmp)
			return tmp;
	}

	return this;
}

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
	m_image->registerListener(m_cb);
	m_position.width = m_position.height = 16;
}

CJiraIconNode::~CJiraIconNode()
{
	m_image->unregisterListener(m_cb);
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
	StyleSaver saver{ painter, this };

	painter->paintString(m_data[Attr::Text]);
}

void CJiraTextNode::measure(IJiraPainter* painter)
{
	StyleSaver saver{ painter, this };

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

CJiraReportNode::CJiraReportNode(const std::shared_ptr<jira::report>& dataset, const std::shared_ptr<std::vector<size_t>>& columns)
	: m_dataset(dataset)
	, m_columns(columns)
{
}

CJiraRowProxy::CJiraRowProxy(size_t id, const std::shared_ptr<jira::report>& dataset, const std::shared_ptr<std::vector<size_t>>& columns)
	: CJiraReportNode(dataset, columns)
	, m_id(id)
{
	CJiraNode::setClass(rules::tableRow);
	auto& record = dataset->data.at(m_id);
	m_proxy = static_cast<CJiraNode*>(record.getRow());
}

std::string CJiraRowProxy::text() const
{
	auto lock = m_dataset.lock();
	if (!lock)
		return{};
	return m_proxy->text();
}

void CJiraRowProxy::setTooltip(const std::string& text)
{
	auto lock = m_dataset.lock();
	if (!lock)
		return;
	m_proxy->setTooltip(text);
}

void CJiraRowProxy::addChild(std::unique_ptr<jira::node>&& child)
{
	auto lock = m_dataset.lock();
	if (!lock)
		return;
	m_proxy->addChild(std::move(child));
}

void CJiraRowProxy::setClass(jira::styles style)
{
	auto lock = m_dataset.lock();
	if (!lock)
		return;
	m_proxy->setClass(style);
}

jira::styles CJiraRowProxy::getStyles() const
{
	auto lock = m_dataset.lock();
	if (!lock)
		return jira::styles::unset;
	return m_proxy->getStyles();
}

const std::vector<std::unique_ptr<jira::node>>& CJiraRowProxy::values() const
{
	auto lock = m_dataset.lock();
	if (!lock) {
		static std::vector<std::unique_ptr<jira::node>> dummy;
		return dummy;
	}
	return m_proxy->values();
}

void CJiraRowProxy::paint(IJiraPainter* painter)
{
	auto lock = m_dataset.lock();
	if (!lock)
		return;
	return m_proxy->paint(painter);
}

void CJiraRowProxy::measure(IJiraPainter* painter)
{
	auto lock = m_dataset.lock();
	if (!lock)
		return;
	m_proxy->measure(painter);

	auto it = m_columns->begin();
	for (auto& node : values()) {
		auto width = cast(node)->getSize().width;
		if (*it < width)
			*it = width;
		*it++;
	}

	auto size = m_proxy->getSize();
	m_position.width = size.width;
	m_position.height = size.height;
}

void CJiraRowProxy::setPosition(int x, int y)
{
	m_position.x = x;
	m_position.y = y;

	auto lock = m_dataset.lock();
	if (!lock)
		return;
	m_proxy->setPosition(0, 0);
}

IJiraNode* CJiraRowProxy::getParent() const
{
	return m_parent;
}

void CJiraRowProxy::setParent(IJiraNode* parent_)
{
	m_parent = parent_;

	auto lock = m_dataset.lock();
	if (!lock)
		return;
	m_proxy->setParent(this);
}

void CJiraRowProxy::repositionChildren()
{
	int x = CELL_MARGIN;
	auto it = m_columns->begin();
	for (auto& node : values()) {
		cast(node)->setPosition(x, 0);
		x += *it++ + 2 * CELL_MARGIN;
	}
	m_position.width = x + CELL_MARGIN;
	m_position.height = m_proxy->getSize().height;
}

jira::node* CJiraRowProxy::findHovered(int x, int y)
{
	x -= m_position.x;
	y -= m_position.y;

	if (x < 0 || (size_t)x > m_position.width ||
		y < 0 || (size_t)y > m_position.height)
		return nullptr;

	auto lock = m_dataset.lock();
	if (!lock)
		return this;

	for (auto& node : values()) {
		auto tmp = cast(node)->findHovered(x, y);
		if (tmp)
			return tmp;
	}

	return this;
}

CJiraHeaderNode::CJiraHeaderNode(const std::shared_ptr<jira::report>& dataset, const std::shared_ptr<std::vector<size_t>>& columns)
	: CJiraReportNode(dataset, columns)
{
	CJiraNode::setClass(rules::tableHead);

	for (auto& col : dataset->schema.cols()) {
		auto name = col->title();
		auto node = std::make_unique<CJiraTextNode>(name);

		auto tooltip = col->titleFull();
		if (name != tooltip)
			node->setTooltip(tooltip);

		CJiraReportNode::addChild(std::move(node));
	}
}

void CJiraHeaderNode::addChild(std::unique_ptr<jira::node>&& /*child*/)
{
	// noop
}

void CJiraHeaderNode::measure(IJiraPainter* painter)
{
	CJiraReportNode::measure(painter);

	auto it = m_columns->begin();
	for (auto& node : values()) {
		*it++ = cast(node)->getSize().width;
	}
}

void CJiraHeaderNode::repositionChildren()
{
	int x = CELL_MARGIN;
	auto it = m_columns->begin();
	for (auto& node : values()) {
		cast(node)->setPosition(x, 0);
		x += *it++ + 2 * CELL_MARGIN;
	}
	m_position.width = x + CELL_MARGIN;
}

CJiraReportTableNode::CJiraReportTableNode(const std::shared_ptr<jira::report>& dataset)
	: m_columns(std::make_shared<std::vector<size_t>>(dataset->schema.cols().size()))
{
	CJiraNode::addChild(std::make_unique<CJiraHeaderNode>(dataset, m_columns));

	auto size = dataset->data.size();
	for (size_t id = 0; id < size; ++id) {
		CJiraNode::addChild(std::make_unique<CJiraRowProxy>(id, dataset, m_columns));
	}
}

void CJiraReportTableNode::addChild(std::unique_ptr<jira::node>&& /*child*/)
{
	// noop
}

void CJiraReportTableNode::measure(IJiraPainter* painter)
{
	StyleSaver saver{ painter, this };

	size_t height = 0;
	for (auto& node : m_children) {
		cast(node)->measure(painter);

		auto nheight = cast(node)->getSize().height;
		cast(node)->setPosition(0, height);
		height += nheight * 12 / 10; // 120%
	}

	for (auto& node : m_children)
		static_cast<CJiraReportNode*>(node.get())->repositionChildren();

	m_position.height = height;
	m_position.width = cast(m_children[0])->getSize().width;
}

CJiraReportElement::CJiraReportElement(const std::shared_ptr<jira::report>& dataset, const jira::server& server, const std::function<void(int, int, int, int)>& invalidator)
	: m_dataset(dataset)
	, m_invalidator(invalidator)
{
	{
		auto text = server.login() + "@" + server.displayName();
		auto title = std::make_unique<CJiraTextNode>(text);
		title->setClass(rules::header);
		CJiraNode::addChild(std::move(title));
	}

	for (auto& error : server.errors()) {
		auto note = std::make_unique<CJiraTextNode>(error);
		note->setClass(rules::error);
		CJiraNode::addChild(std::move(note));
	}

	if (dataset) {
		CJiraNode::addChild(std::make_unique<CJiraReportTableNode>(dataset));

		std::ostringstream o;
		auto low = dataset->data.empty() ? 0 : 1;
		o << "(Issues " << (dataset->startAt + low)
			<< '-' << (dataset->startAt + dataset->data.size())
			<< " of " << dataset->total << ")";

		auto note = std::make_unique<CJiraTextNode>(o.str());
		note->setClass(rules::classSummary);
		CJiraNode::addChild(std::move(note));
	} else {
		auto note = std::make_unique<CJiraTextNode>("Empty");
		note->setClass(rules::classEmpty);
		CJiraNode::addChild(std::move(note));
	}
}

void CJiraReportElement::addChild(std::unique_ptr<jira::node>&& /*child*/)
{
	// noop
}

void CJiraReportElement::measure(IJiraPainter* painter)
{
	StyleSaver saver{ painter, this };

	size_t height = 0;
	size_t width = 0;
	for (auto& node : m_children) {
		cast(node)->measure(painter);

		auto size = cast(node)->getSize();
		cast(node)->setPosition(0, height);
		height += size.height;
		if (width < size.width)
			width = size.width;
	}

	m_position.height = height;
	m_position.width = width;
}

void CJiraReportElement::invalidate(int x, int y, size_t width, size_t height)
{
	x += m_position.x;
	y += m_position.y;
	if (m_invalidator)
		m_invalidator(x, y, width, height);
}
