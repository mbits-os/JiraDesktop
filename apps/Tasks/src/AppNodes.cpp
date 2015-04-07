#include "stdafx.h"

#ifdef max
#undef max
#endif

#include "AppNodes.h"
#include <limits>
#include <sstream>

#include <net/utf8.hpp>
#include "shellapi.h"

using namespace styles::literals;

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

const std::vector<std::string>& CJiraNode::getClassNames() const
{
	return m_classes;
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

	paintThis(painter);

	// slow, but working:
	for (auto& node : m_children) {
		gui::push_origin push{ painter };
		painter->moveOrigin(node->getPosition());
		node->paint(painter);
	}
}

static long double calculated(const styles::rule_storage& rules,
	gui::painter* painter, styles::length_prop prop)
{
	if (!rules.has(prop))
		return 0.0;

	auto u = rules.get(prop);
	ATLASSERT(u.which() == styles::length_u::first_type);

	return painter->dpiRescale(u.first().value());
};

void CJiraNode::measure(gui::painter* painter)
{
	calculateStyles();

	auto styles = calculatedStyle();
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

	auto here = measureThis(painter);
	if (height < here.height)
		height = here.height;
	if (width < here.width)
		width = here.width;

	long double dheight = 0.5;
	dheight += calculated(*styles, painter, styles::prop_border_top_width);
	dheight += calculated(*styles, painter, styles::prop_padding_top);
	dheight += calculated(*styles, painter, styles::prop_padding_bottom);
	dheight += calculated(*styles, painter, styles::prop_border_bottom_width);

	long double dwidth = 0.5;
	dwidth += calculated(*styles, painter, styles::prop_border_left_width);
	dwidth += calculated(*styles, painter, styles::prop_padding_left);
	dwidth += calculated(*styles, painter, styles::prop_padding_right);
	dwidth += calculated(*styles, painter, styles::prop_border_right_width);

	m_position.height = height + (int)dheight;
	m_position.width = width + (int)dwidth;
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

void CJiraNode::paintThis(gui::painter* /*painter*/)
{
}

gui::node::size CJiraNode::measureThis(gui::painter* /*painter*/)
{
	return{ 0, 0 };
}

styles::pointer CJiraNode::getCursor() const
{
	auto styles = calculatedStyle();
	if (styles) {
		if (styles->has(styles::prop_cursor)) {
			auto c = styles->get(styles::prop_cursor);
			if (c != styles::pointer::inherited)
				return c;
		}
	}

	auto parent = m_parent.lock();
	if (parent)
		return parent->getCursor();

	return styles::pointer::inherited;
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

std::shared_ptr<styles::rule_storage> CJiraNode::calculatedStyle() const
{
	if (getHovered()) {
		return getActive() ? m_calculatedHoverActive : m_calculatedHover;
	}

	return getActive() ? m_calculatedActive : m_calculated;
}

std::shared_ptr<styles::rule_storage> CJiraNode::normalCalculatedStyles() const
{
	return m_calculated;
}

std::shared_ptr<styles::stylesheet> CJiraNode::styles() const
{
	return m_allApplying;
}

void CJiraNode::applyStyles(const std::shared_ptr<styles::stylesheet>& stylesheet)
{
	m_calculated.reset();
	m_calculatedHover.reset();
	m_calculatedActive.reset();
	m_calculatedHoverActive.reset();
	m_allApplying = std::make_shared<styles::stylesheet>();

	for (auto& rules : stylesheet->m_rules) {
		if (rules->m_sel.maySelect(this))
			m_allApplying->m_rules.push_back(rules);
	}

	for (auto& node : children())
		node->applyStyles(stylesheet);
}

styles::pixels calculatedFontSize(gui::node* node)
{
	using namespace styles;

	if (!node)
		return 14_px;

	auto styles = node->normalCalculatedStyles();
	if (styles && styles->has(prop_font_size)) {
		auto u = styles->get(prop_font_size);
		ATLASSERT(u.which() == length_u::first_type);
		return u.first();
	}

	return calculatedFontSize(node->getParent().get());
}

styles::pixels parentFontSize(gui::node* node)
{
	return calculatedFontSize(node->getParent().get());
}

styles::pixels calculate(styles::rule_storage& rules,
	styles::length_prop prop,
	const styles::pixels& px)
{
	using namespace styles;
	if (rules.has(prop)) {
		auto u = rules.get(prop);

		if (u.which() == styles::length_u::first_type)
			return u.first();

		if (u.which() == styles::length_u::second_type) {
			auto em = u.second();
			auto ret = em.value(px);
			rules <<= def::rule(prop, ret);
			return ret;
		}
	}

	return px;
}

void calculate(styles::rule_storage& rules, gui::node* node)
{
	using namespace styles;
	auto fontSize = calculate(rules, prop_font_size, parentFontSize(node));
	calculate(rules, styles::prop_border_top_width, fontSize);
	calculate(rules, styles::prop_border_right_width, fontSize);
	calculate(rules, styles::prop_border_bottom_width, fontSize);
	calculate(rules, styles::prop_border_left_width, fontSize);
	calculate(rules, prop_padding_top, fontSize);
	calculate(rules, prop_padding_right, fontSize);
	calculate(rules, prop_padding_bottom, fontSize);
	calculate(rules, prop_padding_left, fontSize);
}

void CJiraNode::calculateStyles()
{
	styles::rule_storage
		normal,
		hover,
		active;

	for (auto& rule : m_allApplying->m_rules) {
		if (rule->m_sel.m_pseudoClass == styles::pseudo::hover)
			hover <<= *rule;
		else if (rule->m_sel.m_pseudoClass == styles::pseudo::active)
			active <<= *rule;
		else
			normal <<= *rule;
	}

	calculate(normal, this);
	calculate(hover, this);
	calculate(active, this);

	m_calculated = std::make_shared<styles::rule_storage>(normal);

	if (hover.empty())
		m_calculatedHover = m_calculated;
	else {
		m_calculatedHover = std::make_shared<styles::rule_storage>(normal);
		*m_calculatedHover <<= hover;
	}

	if (active.empty()) {
		m_calculatedActive = m_calculated;
		m_calculatedHoverActive = m_calculatedHover;
	} else {
		m_calculatedActive = std::make_shared<styles::rule_storage>(normal);
		*m_calculatedActive <<= active;

		if (hover.empty())
			m_calculatedHoverActive = m_calculatedActive;
		else {
			m_calculatedHoverActive = std::make_shared<styles::rule_storage>(*m_calculatedHover);
			*m_calculatedHoverActive <<= active;
		}
	}
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

void CJiraIconNode::paintThis(gui::painter* painter)
{
	painter->paintImage(m_image.get(), m_position.width, m_position.height);
}

gui::node::size CJiraIconNode::measureThis(gui::painter* painter)
{
	auto size = painter->dpiRescale(16);
	return{ (size_t)size, (size_t)size };
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

void CJiraUserNode::paintThis(gui::painter* painter)
{
	painter->paintImage(m_image.get(), m_position.width, m_position.height);
}

gui::node::size CJiraUserNode::measureThis(gui::painter* painter)
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
		return{ size, size };

	m_selectedSize = selected;

	if (m_image)
		m_image->unregisterListener(m_cb);
	m_image.reset();

	m_cb->parent = shared_from_this();
	auto doc = m_document.lock();
	if (doc)
		m_image = doc->createImage(image);

	if (m_image)
		m_image->registerListener(m_cb);

	return{ size, size };
}

CJiraLinkNode::CJiraLinkNode(const std::string& href)
	: CJiraNode(gui::elem::link)
{
	m_data[Attr::Href] = href;
}

CJiraTextNode::CJiraTextNode(const std::string& text)
	: CJiraNode(gui::elem::text)
{
	m_data[Attr::Text] = text;
}

void CJiraTextNode::paintThis(gui::painter* painter)
{
	painter->paintString(m_data[Attr::Text]);
}

gui::node::size CJiraTextNode::measureThis(gui::painter* painter)
{
	return painter->measureString(m_data[Attr::Text]);
}

CJiraDocument::CJiraDocument(std::shared_ptr<gui::image_ref>(*creator)(const std::shared_ptr<jira::server>&, const std::string&))
	: m_creator(creator)
{
}

void CJiraDocument::setCurrent(const std::shared_ptr<jira::server>& server)
{
	m_server = server;
}

std::shared_ptr<gui::node> CJiraDocument::createTable()
{
	return std::make_shared<CJiraTableNode>();
}

std::shared_ptr<gui::node> CJiraDocument::createTableHead()
{
	return std::make_shared<CJiraTableRowNode>(gui::elem::table_head);
}

std::shared_ptr<gui::node> CJiraDocument::createTableRow()
{
	return std::make_shared<CJiraTableRowNode>(gui::elem::table_row);
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

void CJiraTableNode::addChild(const std::shared_ptr<gui::node>& child)
{
	auto elem = child->getNodeName();
	if (elem == gui::elem::table_row || elem == gui::elem::table_head) {
		CJiraNode::addChild(child);
		std::static_pointer_cast<CJiraTableRowNode>(child)->setColumns(m_columns);
	}
}

void CJiraTableNode::measure(gui::painter* painter)
{
	calculateStyles();
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

		node->setPosition(0, height);
		height += node->getSize().height;
	}

	for (auto& node : m_children)
		static_cast<CJiraTableRowNode*>(node.get())->repositionChildren();

	m_position.height = height;
	m_position.width = m_children.empty() ? 0 : m_children[0]->getSize().width;
}

gui::node::size CJiraTableNode::measureThis(gui::painter* /*painter*/)
{
	return{ 0, 0 };
}

CJiraTableRowNode::CJiraTableRowNode(gui::elem name)
	: CJiraNode(name)
{
}

void CJiraTableRowNode::setColumns(const std::shared_ptr<std::vector<size_t>>& columns)
{
	m_columns = columns;
}

gui::node::size CJiraTableRowNode::measureThis(gui::painter* /*painter*/)
{
	if (!m_columns)
		return{ 0,0 };

	auto it = m_columns->begin();
	for (auto& node : children()) {
		auto sz = node->getSize();
		if (*it < sz.width)
			*it = sz.width;
		++it;
	}

	return{ 0,0 };
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
		auto table = std::make_shared<CJiraTableNode>();

		{
			auto header = std::make_shared<CJiraTableRowNode>(gui::elem::table_head);
			for (auto& col : dataset->schema.cols()) {
				auto name = col->title();
				auto node = std::make_shared<CJiraTextNode>(name);

				auto tooltip = col->titleFull();
				if (name != tooltip)
					node->setTooltip(tooltip);
				header->addChild(node);
			}
			table->addChild(header);
		}

		for (auto& record : dataset->data)
			table->addChild(record.getRow());

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

gui::node::size CJiraReportElement::measureThis(gui::painter* painter)
{
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

	return{ width, height };
}

void CJiraReportElement::invalidate(int x, int y, size_t width, size_t height)
{
	x += m_position.x;
	y += m_position.y;
	if (m_invalidator)
		m_invalidator(x, y, width, height);
}
