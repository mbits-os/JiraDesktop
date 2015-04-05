#include "stdafx.h"

#ifdef max
#undef max
#endif

#include "AppNodes.h"
#include <limits>
#include <sstream>

#include <net/utf8.hpp>
#include "shellapi.h"

enum {
	CELL_MARGIN = 7
};

CJiraNode::CJiraNode(gui::elem name)
	: m_nodeName(name)
{
}

gui::elem CJiraNode::getNodeName() const
{
	return m_nodeName;
}

void CJiraNode::addClass(const std::string& name)
{
	auto it = std::find(std::begin(m_classes), std::end(m_classes), name);
	if (it == std::end(m_classes))
		m_classes.push_back(name);
}

void CJiraNode::removeClass(const std::string& name)
{
	auto it = std::find(std::begin(m_classes), std::end(m_classes), name);
	if (it != std::end(m_classes))
		m_classes.erase(it);
}

bool CJiraNode::hasClass(const std::string& name) const
{
	auto it = std::find(std::begin(m_classes), std::end(m_classes), name);
	return it != std::end(m_classes);
}

std::string CJiraNode::text() const
{
	auto it = m_data.find(Attr::Text);
	if (it != m_data.end())
		return it->second;

	std::string out;
	for (auto& child : m_children) {
		out += child->text();
	}
	return out;
}

void CJiraNode::setTooltip(const std::string& text)
{
	m_data[Attr::Tooltip] = text;
}

void CJiraNode::addChild(const std::shared_ptr<node>& child)
{
	auto it = m_data.find(Attr::Text);
	if (it != m_data.end())
		return;

	child->setParent(shared_from_this());
	m_children.push_back(std::move(child));
}

const std::vector<std::shared_ptr<gui::node>>& CJiraNode::children() const
{
	return m_children;
}

void CJiraNode::paint(gui::painter* painter)
{
	StyleSaver saver{ painter, this };

	// slow, but working:
	for (auto& node : m_children) {
		gui::push_origin push{ painter };
		painter->moveOrigin(node->getPosition());
		node->paint(painter);
	}
}

void CJiraNode::measure(gui::painter* painter)
{
	StyleSaver saver{ painter, this };

	size_t height = 0;
	size_t width = 0;
	int x = 0;
	for (auto& node : m_children) {
		node->measure(painter);
		auto ret = node->getSize();
		if (height < ret.height)
			height = ret.height;

		width += ret.width;
		node->setPosition(x, 0);
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

gui::node::point CJiraNode::getPosition()
{
	return{ m_position.x, m_position.y };
}

gui::node::point CJiraNode::getAbsolutePos()
{
	auto parent = m_parent.lock();
	if (!parent)
		return getPosition();
	auto pt = parent->getAbsolutePos();
	return{ pt.x + m_position.x, pt.y + m_position.y };
}

gui::node::size CJiraNode::getSize()
{
	return{ m_position.width, m_position.height };
}

std::shared_ptr<gui::node> CJiraNode::getParent() const
{
	return m_parent.lock();
}

void CJiraNode::setParent(const std::shared_ptr<gui::node>& node)
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
	auto parent = m_parent.lock();
	if (parent)
		parent->invalidate(x, y, width, height);
}

std::shared_ptr<gui::node> CJiraNode::nodeFromPoint(int x, int y)
{
	x -= m_position.x;
	y -= m_position.y;

	if (x < 0 || (size_t)x > m_position.width ||
		y < 0 || (size_t)y > m_position.height)
		return nullptr;

	for (auto& node : m_children) {
		auto tmp = node->nodeFromPoint(x, y);
		if (tmp)
			return tmp;
	}

	return shared_from_this();
}

void CJiraNode::setHovered(bool hovered)
{
	bool changed = false;
	if (hovered) {
		auto value = ++m_hoverCount;
		changed = value == 1;
	} else {
		auto value = --m_hoverCount;
		changed = value == 0;
	}

	if (changed) {
		invalidate();
		auto parent = getParent();
		if (parent)
			parent->setHovered(hovered);
	}
}

bool CJiraNode::getHovered() const
{
	return m_hoverCount > 0;
}

void CJiraNode::setActive(bool active)
{
	bool changed = false;
	if (active) {
		auto value = ++m_activeCount;
		changed = value == 1;
	} else {
		auto value = --m_activeCount;
		changed = value == 0;
	}

	if (changed) {
		invalidate();
		auto parent = getParent();
		if (parent)
			parent->setActive(active);
	}
}

bool CJiraNode::getActive() const
{
	return m_activeCount > 0;
}

void CJiraNode::activate()
{
	auto it = m_data.find(Attr::Href);
	if (it != m_data.end() && !it->second.empty()) {
		openLink(it->second);
		return;
	}

	auto parent = m_parent.lock();
	if (parent)
		parent->activate();
}

void CJiraNode::openLink(const std::string& url)
{
	ShellExecute(nullptr, nullptr, utf::widen(url).c_str(), nullptr, nullptr, SW_SHOW);
}

void CJiraNode::setCursor(gui::cursor c)
{
	m_cursor = c;
}

gui::cursor CJiraNode::getCursor() const
{
	if (m_cursor != gui::cursor::inherited)
		return m_cursor;

	auto parent = m_parent.lock();
	if (parent)
		return parent->getCursor();

	return gui::cursor::arrow;
}

bool CJiraNode::hasTooltip() const
{
	auto it = m_data.find(Attr::Tooltip);
	return it != m_data.end();
}

const std::string& CJiraNode::getTooltip() const
{
	auto it = m_data.find(Attr::Tooltip);
	if (it != m_data.end())
		return it->second;

	static std::string dummy;
	return dummy;
}

void ImageCb::onImageChange(gui::image_ref*)
{
	auto par = parent.lock();
	if (par)
		par->invalidate();
}

CJiraIconNode::CJiraIconNode(const std::string& uri, const std::shared_ptr<gui::image_ref>& image, const std::string& tooltip)
	: CJiraNode(gui::elem::image)
	, m_image(image)
	, m_cb(std::make_shared<ImageCb>())
{
	m_data[Attr::Href] = uri;
	CJiraNode::setTooltip(tooltip);
	m_position.width = m_position.height = 16;
}

CJiraIconNode::~CJiraIconNode()
{
	m_image->unregisterListener(m_cb);
	m_cb->parent.reset();
}

void CJiraIconNode::attach()
{
	m_cb->parent = shared_from_this();
	m_image->registerListener(m_cb);
}

void CJiraIconNode::addChild(const std::shared_ptr<node>& /*child*/)
{
	 // noop
}

void CJiraIconNode::paint(gui::painter* painter)
{
	painter->paintImage(m_image.get(), m_position.width, m_position.height);
}

void CJiraIconNode::measure(gui::painter* painter)
{
	m_position.width = m_position.height = painter->dpiRescale(16);
}

CJiraUserNode::CJiraUserNode(const std::weak_ptr<CJiraDocument>& document, std::map<uint32_t, std::string>&& avatar, const std::string& tooltip)
	: CJiraNode(gui::elem::icon)
	, m_document(document)
	, m_cb(std::make_shared<ImageCb>())
	, m_urls(std::move(avatar))
	, m_selectedSize(0)
{
	CJiraNode::setTooltip(tooltip);
	m_position.width = m_position.height = 16;
}

CJiraUserNode::~CJiraUserNode()
{
	m_image->unregisterListener(m_cb);
	m_cb->parent.reset();
}

void CJiraUserNode::addChild(const std::shared_ptr<node>& /*child*/)
{
	// noop
}

void CJiraUserNode::paint(gui::painter* painter)
{
	painter->paintImage(m_image.get(), m_position.width, m_position.height);
}

void CJiraUserNode::measure(gui::painter* painter)
{
	auto size = (size_t)painter->dpiRescale(16);
	auto selected = 0;

	std::string image;
	auto it = m_urls.find(size);
	if (it != m_urls.end()) {
		image = it->second;
		selected = it->first;
	} else {
		size_t delta = std::numeric_limits<size_t>::max();
		for (auto& pair : m_urls) {
			if (pair.first < size)
				continue;

			size_t d =  pair.first - size;
			if (d < delta) {
				delta = d;
				image = pair.second;
				selected = pair.first;
			}
		}
		for (auto& pair : m_urls) {
			if (pair.first > size)
				continue;

			size_t d = size - pair.first;
			if (d < delta) {
				delta = d;
				image = pair.second;
				selected = pair.first;
			}
		}
	}

	if (selected == m_selectedSize)
		return;

	m_selectedSize = selected;
	m_position.width = m_position.height = size;

	if (m_image)
		m_image->unregisterListener(m_cb);
	m_image.reset();

	m_cb->parent = shared_from_this();
	auto doc = m_document.lock();
	if (doc)
		m_image = doc->createImage(image);

	if (m_image)
		m_image->registerListener(m_cb);
}

CJiraLinkNode::CJiraLinkNode(const std::string& href)
	: CJiraNode(gui::elem::link)
{
	m_data[Attr::Href] = href;
	CJiraNode::setCursor(gui::cursor::pointer);
}

CJiraTextNode::CJiraTextNode(const std::string& text)
	: CJiraNode(gui::elem::text)
{
	m_data[Attr::Text] = text;
}

void CJiraTextNode::paint(gui::painter* painter)
{
	StyleSaver saver{ painter, this };

	painter->paintString(m_data[Attr::Text]);
}

void CJiraTextNode::measure(gui::painter* painter)
{
	StyleSaver saver{ painter, this };

	auto size = painter->measureString(m_data[Attr::Text]);
	m_position.width = size.width;
	m_position.height = size.height;
}

CJiraDocument::CJiraDocument(std::shared_ptr<gui::image_ref>(*creator)(const std::shared_ptr<jira::server>&, const std::string&))
	: m_creator(creator)
{
}

void CJiraDocument::setCurrent(const std::shared_ptr<jira::server>& server)
{
	m_server = server;
}

std::shared_ptr<gui::node> CJiraDocument::createTableRow()
{
	return std::make_shared<CJiraNode>(gui::elem::span); // TODO: should be report row
}

std::shared_ptr<gui::node> CJiraDocument::createEmpty()
{
	return std::make_shared<CJiraNode>(gui::elem::span);
}

std::shared_ptr<gui::node> CJiraDocument::createSpan()
{
	return std::make_shared<CJiraNode>(gui::elem::span);
}

std::shared_ptr<gui::node> CJiraDocument::createIcon(const std::string& uri, const std::string& text, const std::string& description)
{
	auto image = createImage(uri);
	auto tooltip = text;
	if (text.empty() || description.empty())
		tooltip += description;
	else
		tooltip += "\n" + description;

	auto icon = std::make_shared<CJiraIconNode>(uri, image, tooltip);
	icon->attach();
	return icon;
}

std::shared_ptr<gui::node> CJiraDocument::createUser(bool /*active*/, const std::string& display, const std::string& email, const std::string& /*login*/, std::map<uint32_t, std::string>&& avatar)
{
	if (avatar.empty()) {
		auto node = createText(display);
		if (!email.empty())
			node->setTooltip(email);
		return std::move(node);
	}

	auto tooltip = display;
	if (display.empty() || email.empty())
		tooltip += email;
	else
		tooltip += "\n" + email;

	return std::make_shared<CJiraUserNode>(shared_from_this(), std::move(avatar), tooltip);
}

std::shared_ptr<gui::node> CJiraDocument::createLink(const std::string& href)
{
	return std::make_shared<CJiraLinkNode>(href);
}

std::shared_ptr<gui::node> CJiraDocument::createText(const std::string& text)
{
	return std::make_shared<CJiraTextNode>(text);
}

std::shared_ptr<gui::image_ref> CJiraDocument::createImage(const std::string& uri)
{
	std::lock_guard<std::mutex> lock(m_guard);
	auto it = m_cache.lower_bound(uri);
	if (it != m_cache.end() && it->first == uri)
		return it->second;

	auto image = m_creator(m_server, uri);
	m_cache.insert(it, std::make_pair(uri, image));
	return image;
}

CJiraTableNode::CJiraTableNode()
	: CJiraNode(gui::elem::table)
	, m_columns(std::make_shared<std::vector<size_t>>())
{
}

std::shared_ptr<gui::node> CJiraTableNode::addHeader()
{
	auto node = std::make_shared<CJiraTableRowNode>(gui::elem::table_head, m_columns);
	CJiraNode::addChild(node);
	return node;
}

std::shared_ptr<gui::node> CJiraTableNode::addRow()
{
	auto node = std::make_shared<CJiraTableRowNode>(gui::elem::table_row, m_columns);
	CJiraNode::addChild(node);
	return node;
}

void CJiraTableNode::addChild(const std::shared_ptr<gui::node>& /*child*/)
{
	// noop
}

void CJiraTableNode::measure(gui::painter* painter)
{
	StyleSaver saver{ painter, this };

	size_t columns = 0;
	for (auto& node : m_children) {
		auto values = node->children().size();
		if (columns < values)
			columns = values;
	}

	m_columns->assign(columns, 0);

	size_t height = 0;
	for (auto& node : m_children) {
		node->measure(painter);

		auto nheight = node->getSize().height;
		node->setPosition(0, height);
		height += nheight * 12 / 10; // 120%
	}

	for (auto& node : m_children)
		static_cast<CJiraTableRowNode*>(node.get())->repositionChildren();

	m_position.height = height;
	m_position.width = m_children.empty() ? 0 : m_children[0]->getSize().width;
}

CJiraTableRowNode::CJiraTableRowNode(gui::elem name, const std::shared_ptr<std::vector<size_t>>& columns)
	: CJiraNode(name)
	, m_columns(columns)
{
}

void CJiraTableRowNode::measure(gui::painter* painter)
{
	CJiraNode::measure(painter);

	auto it = m_columns->begin();
	for (auto& node : children()) {
		auto sz = node->getSize();
		if (*it < sz.width)
			*it = sz.width;
		++it;
	}
}

void CJiraTableRowNode::repositionChildren()
{
	int x = CELL_MARGIN;
	auto it = m_columns->begin();
	for (auto& node : children()) {
		node->setPosition(x, 0);
		x += *it++ + 2 * CELL_MARGIN;
	}
	m_position.width = x + CELL_MARGIN;
}
	
CJiraReportNode::CJiraReportNode(gui::elem name, const std::shared_ptr<jira::report>& dataset, const std::shared_ptr<std::vector<size_t>>& columns)
	: CJiraNode(name)
	, m_dataset(dataset)
	, m_columns(columns)
{
}

CJiraRowProxy::CJiraRowProxy(size_t id, const std::shared_ptr<jira::report>& dataset, const std::shared_ptr<std::vector<size_t>>& columns)
	: CJiraReportNode(gui::elem::table_row, dataset, columns)
	, m_id(id)
{
	auto& record = dataset->data.at(m_id);
	m_proxy = record.getRow();
}

std::string CJiraRowProxy::text() const
{
	auto lock = m_dataset.lock();
	if (!lock)
		return{};
	return m_proxy->text();
}

void CJiraRowProxy::addClass(const std::string& name)
{
	auto lock = m_dataset.lock();
	if (!lock)
		return;
	m_proxy->addClass(name);
}

void CJiraRowProxy::removeClass(const std::string& name)
{
	auto lock = m_dataset.lock();
	if (!lock)
		return;
	m_proxy->removeClass(name);
}

bool CJiraRowProxy::hasClass(const std::string& name) const
{
	auto lock = m_dataset.lock();
	if (!lock)
		return false;
	return m_proxy->hasClass(name);
}

void CJiraRowProxy::setTooltip(const std::string& text)
{
	auto lock = m_dataset.lock();
	if (!lock)
		return;
	m_proxy->setTooltip(text);
}

void CJiraRowProxy::addChild(const std::shared_ptr<gui::node>& child)
{
	auto lock = m_dataset.lock();
	if (!lock)
		return;
	m_proxy->addChild(std::move(child));
}

const std::vector<std::shared_ptr<gui::node>>& CJiraRowProxy::children() const
{
	auto lock = m_dataset.lock();
	if (!lock) {
		static std::vector<std::shared_ptr<gui::node>> dummy;
		return dummy;
	}
	return m_proxy->children();
}

void CJiraRowProxy::paint(gui::painter* painter)
{
	auto lock = m_dataset.lock();
	if (!lock)
		return;
	CJiraReportNode::paint(painter);
	m_proxy->paint(painter);
}

void CJiraRowProxy::measure(gui::painter* painter)
{
	auto lock = m_dataset.lock();
	if (!lock)
		return;
	m_proxy->measure(painter);

	auto it = m_columns->begin();
	for (auto& node : children()) {
		auto width = node->getSize().width;
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

std::shared_ptr<gui::node> CJiraRowProxy::getParent() const
{
	return m_parent.lock();
}

void CJiraRowProxy::setParent(const std::shared_ptr<gui::node>& parent_)
{
	m_parent = parent_;

	auto lock = m_dataset.lock();
	if (!lock)
		return;
	m_proxy->setParent(shared_from_this());
}

void CJiraRowProxy::repositionChildren()
{
	int x = CELL_MARGIN;
	auto it = m_columns->begin();
	for (auto& node : children()) {
		node->setPosition(x, 0);
		x += *it++ + 2 * CELL_MARGIN;
	}
	m_position.width = x + CELL_MARGIN;
	m_position.height = m_proxy->getSize().height;
}

std::shared_ptr<gui::node> CJiraRowProxy::nodeFromPoint(int x, int y)
{
	x -= m_position.x;
	y -= m_position.y;

	if (x < 0 || (size_t)x > m_position.width ||
		y < 0 || (size_t)y > m_position.height)
		return nullptr;

	auto lock = m_dataset.lock();
	if (!lock)
		return shared_from_this();

	for (auto& node : children()) {
		auto tmp = node->nodeFromPoint(x, y);
		if (tmp)
			return tmp;
	}

	return shared_from_this();
}

CJiraHeaderNode::CJiraHeaderNode(const std::shared_ptr<jira::report>& dataset, const std::shared_ptr<std::vector<size_t>>& columns)
	: CJiraReportNode(gui::elem::table_head, dataset, columns)
{
}

void CJiraHeaderNode::addChildren()
{
	auto dataset = m_dataset.lock();
	for (auto& col : dataset->schema.cols()) {
		auto name = col->title();
		auto node = std::make_shared<CJiraTextNode>(name);

		auto tooltip = col->titleFull();
		if (name != tooltip)
			node->setTooltip(tooltip);

		CJiraReportNode::addChild(std::move(node));
	}
}

void CJiraHeaderNode::addChild(const std::shared_ptr<gui::node>& /*child*/)
{
	// noop
}

void CJiraHeaderNode::measure(gui::painter* painter)
{
	CJiraReportNode::measure(painter);

	auto it = m_columns->begin();
	for (auto& node : children()) {
		*it++ = node->getSize().width;
	}
}

void CJiraHeaderNode::repositionChildren()
{
	int x = CELL_MARGIN;
	auto it = m_columns->begin();
	for (auto& node : children()) {
		node->setPosition(x, 0);
		x += *it++ + 2 * CELL_MARGIN;
	}
	m_position.width = x + CELL_MARGIN;
}

CJiraReportTableNode::CJiraReportTableNode(const std::shared_ptr<jira::report>& dataset)
	: CJiraNode(gui::elem::table)
	, m_columns(std::make_shared<std::vector<size_t>>(dataset->schema.cols().size()))
{
}

void CJiraReportTableNode::addChildren(const std::shared_ptr<jira::report>& dataset)
{
	auto header = std::make_shared<CJiraHeaderNode>(dataset, m_columns);
	header->addChildren();
	CJiraNode::addChild(header);

	auto size = dataset->data.size();
	for (size_t id = 0; id < size; ++id) {
		CJiraNode::addChild(std::make_shared<CJiraRowProxy>(id, dataset, m_columns));
	}
}

void CJiraReportTableNode::addChild(const std::shared_ptr<gui::node>& /*child*/)
{
	// noop
}

void CJiraReportTableNode::measure(gui::painter* painter)
{
	StyleSaver saver{ painter, this };

	size_t height = 0;
	for (auto& node : m_children) {
		node->measure(painter);

		auto nheight = node->getSize().height;
		node->setPosition(0, height);
		height += nheight * 12 / 10; // 120%
	}

	for (auto& node : m_children)
		static_cast<CJiraReportNode*>(node.get())->repositionChildren();

	m_position.height = height;
	m_position.width = m_children[0]->getSize().width;
}

CJiraReportElement::CJiraReportElement(const std::shared_ptr<jira::report>& dataset, const std::function<void(int, int, int, int)>& invalidator)
	: CJiraNode(gui::elem::block)
	, m_dataset(dataset)
	, m_invalidator(invalidator)
{
}

void CJiraReportElement::addChildren(const jira::server& server)
{
	{
		auto text = server.login() + "@" + server.displayName();
		auto title = std::make_shared<CJiraTextNode>(text);
		auto block = std::make_shared<CJiraNode>(gui::elem::header);
		block->addChild(title);
		CJiraNode::addChild(block);
	}

	for (auto& error : server.errors()) {
		auto note = std::make_shared<CJiraTextNode>(error);
		note->addClass("error");
		CJiraNode::addChild(std::move(note));
	}

	auto dataset = m_dataset.lock();
	if (dataset) {
		auto table = std::make_shared<CJiraReportTableNode>(dataset);
		table->addChildren(dataset);
		CJiraNode::addChild(table);

		std::ostringstream o;
		auto low = dataset->data.empty() ? 0 : 1;
		o << "(Issues " << (dataset->startAt + low)
			<< '-' << (dataset->startAt + dataset->data.size())
			<< " of " << dataset->total << ")";

		auto note = std::make_shared<CJiraTextNode>(o.str());
		note->addClass("summary");
		CJiraNode::addChild(std::move(note));
	} else {
		auto note = std::make_shared<CJiraTextNode>("Empty");
		note->addClass("empty");
		CJiraNode::addChild(std::move(note));
	}
}

void CJiraReportElement::addChild(const std::shared_ptr<gui::node>& /*child*/)
{
	// noop
}

void CJiraReportElement::measure(gui::painter* painter)
{
	StyleSaver saver{ painter, this };

	size_t height = 0;
	size_t width = 0;
	for (auto& node : m_children) {
		node->measure(painter);

		auto size = node->getSize();
		node->setPosition(0, height);
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
