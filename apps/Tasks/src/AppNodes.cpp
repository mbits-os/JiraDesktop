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
using namespace gui::literals;

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

	auto style = calculatedStyle();
	auto& ref = *style;
	if (ref.has(styles::prop_background)) {
		painter->paintBackground(ref.get(styles::prop_background),
			m_position.size.width, m_position.size.height);
	}

	painter->paintBorder(this);

	paintContents(painter, offsetLeft(), offsetTop());
}

static gui::pixels calculated(const styles::rule_storage& rules,
	styles::length_prop prop)
{
	if (!rules.has(prop))
		return 0.0;

	auto u = rules.get(prop);
	ATLASSERT(u.which() == styles::length_u::first_type);

	return u.first();
};

void CJiraNode::measure(gui::painter* painter)
{
	calculateStyles();
	StyleSaver saver{ painter, this };

	gui::point tl{ offsetLeft(), offsetTop() };
	gui::point br{ offsetRight(), offsetBottom() };
	auto size = measureContents(painter, tl.x, tl.y);

	m_position.size = { tl.x + br.x + size.width, tl.y + br.y + size.height };
}

void CJiraNode::setPosition(const gui::pixels& x, const gui::pixels& y)
{
	m_position.pt = { x, y };
}

gui::point CJiraNode::getPosition()
{
	return m_position.pt;
}

gui::point CJiraNode::getAbsolutePos()
{
	auto parent = m_parent.lock();
	if (!parent)
		return getPosition();
	auto pt = parent->getAbsolutePos();
	return pt + m_position.pt;
}

gui::size CJiraNode::getSize()
{
	return m_position.size;
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
	invalidate({ 0, 0 }, m_position.size);
}

void CJiraNode::invalidate(const gui::point& pt, const gui::size& size)
{
	auto p = pt + m_position.pt;
	auto parent = m_parent.lock();
	if (parent)
		parent->invalidate(p, size);
}

std::shared_ptr<gui::node> CJiraNode::nodeFromPoint(const gui::pixels& x_, const gui::pixels& y_)
{
	auto x = x_ - m_position.pt.x;
	auto y = y_ - m_position.pt.y;

	if (x < 0 || x > m_position.size.width ||
		y < 0 || y > m_position.size.height)
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

void CJiraNode::paintContents(gui::painter* painter,
	const gui::pixels&, const gui::pixels&)
{
	for (auto& node : m_children) {
		gui::push_origin push{ painter };
		painter->moveOrigin(node->getPosition());
		node->paint(painter);
	}
}

gui::pointer CJiraNode::getCursor() const
{
	auto styles = calculatedStyle();
	if (styles) {
		if (styles->has(styles::prop_cursor)) {
			auto c = styles->get(styles::prop_cursor);
			if (c != gui::pointer::inherited)
				return c;
		}
	}

	auto parent = m_parent.lock();
	if (parent)
		return parent->getCursor();

	return gui::pointer::inherited;
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

gui::pixels calculatedFontSize(gui::node* node)
{
	using namespace styles;
	using namespace gui::literals;

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

gui::pixels parentFontSize(gui::node* node)
{
	return calculatedFontSize(node->getParent().get());
}

gui::pixels calculate(styles::rule_storage& rules,
	styles::length_prop prop,
	const gui::pixels& px)
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
	calculate(rules, prop_margin_top, fontSize);
	calculate(rules, prop_margin_right, fontSize);
	calculate(rules, prop_margin_bottom, fontSize);
	calculate(rules, prop_margin_left, fontSize);
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

gui::pixels CJiraNode::offsetLeft() const
{
	auto style = calculatedStyle();
	auto& ref = *style;
	return
		calculated(ref, styles::prop_border_left_width) +
		calculated(ref, styles::prop_padding_left);
}

gui::pixels CJiraNode::offsetTop() const
{
	auto style = calculatedStyle();
	auto& ref = *style;
	return
		calculated(ref, styles::prop_border_top_width) +
		calculated(ref, styles::prop_padding_top);
}

gui::pixels CJiraNode::offsetRight() const
{
	auto style = calculatedStyle();
	auto& ref = *style;
	return
		calculated(ref, styles::prop_border_right_width) +
		calculated(ref, styles::prop_padding_right);
}

gui::pixels CJiraNode::offsetBottom() const
{
	auto style = calculatedStyle();
	auto& ref = *style;
	return
		calculated(ref, styles::prop_border_bottom_width) +
		calculated(ref, styles::prop_padding_bottom);
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
	m_position.size.width = m_position.size.height = 16_px;
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

void CJiraIconNode::paintContents(gui::painter* painter,
		const gui::pixels& offX, const gui::pixels& offY)
{
	gui::push_origin push{ painter };
	painter->moveOrigin({offX, offY});
	painter->paintImage(m_image.get(), 16_px, 16_px);
}

gui::size CJiraIconNode::measureContents(gui::painter*,
	const gui::pixels&, const gui::pixels&)
{
	return { 16_px, 16_px };
}

CJiraUserNode::CJiraUserNode(const std::weak_ptr<CJiraDocument>& document, std::map<uint32_t, std::string>&& avatar, const std::string& tooltip)
	: CJiraNode(gui::elem::icon)
	, m_document(document)
	, m_cb(std::make_shared<ImageCb>())
	, m_urls(std::move(avatar))
	, m_selectedSize(0)
{
	CJiraNode::setTooltip(tooltip);
	m_position.size.width = m_position.size.height = 16_px;
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

void CJiraUserNode::paintContents(gui::painter* painter,
	const gui::pixels& offX, const gui::pixels& offY)
{
	gui::push_origin push{ painter };
	painter->moveOrigin({ offX, offY });
	painter->paintImage(m_image.get(), 16_px, 16_px);
}

gui::size CJiraUserNode::measureContents(gui::painter* painter,
	const gui::pixels&, const gui::pixels&)
{
	auto size = 16_px;
	m_position.size = { size, size };
	auto scaled = (size_t)(painter->trueZoom().scaleL(size.value()));
	auto selected = 0;

	std::string image;
	auto it = m_urls.find(scaled);
	if (it != m_urls.end()) {
		image = it->second;
		selected = it->first;
	} else {
		size_t delta = std::numeric_limits<size_t>::max();
		for (auto& pair : m_urls) {
			if (pair.first < scaled)
				continue;

			size_t d =  pair.first - scaled;
			if (d < delta) {
				delta = d;
				image = pair.second;
				selected = pair.first;
			}
		}
		for (auto& pair : m_urls) {
			if (pair.first > scaled)
				continue;

			size_t d = scaled - pair.first;
			if (d < delta) {
				delta = d;
				image = pair.second;
				selected = pair.first;
			}
		}
	}

	size = m_position.size.width;

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

CJiraBlockNode::CJiraBlockNode(gui::elem name)
	: CJiraNode(name)
{
}

gui::size CJiraBlockNode::measureContents(gui::painter* painter,
	const gui::pixels& offX, const gui::pixels& offY)
{
	gui::size sz;
	auto x = offX;
	auto y = offY;
	for (auto& node : m_children) {
		node->measure(painter);
		auto ret = node->getSize();
		if (sz.width < ret.width)
			sz.width = ret.width;

		node->setPosition(x, y);

		sz.height += ret.height;
		y += ret.height;
	}

	return sz;
}

CJiraSpanNode::CJiraSpanNode(gui::elem name)
	: CJiraNode(name)
{
}

gui::size CJiraSpanNode::measureContents(gui::painter* painter,
	const gui::pixels& offX, const gui::pixels& offY)
{
	gui::size sz;
	auto x = offX;
	auto y = offY;
	for (auto& node : m_children) {
		node->measure(painter);
		auto ret = node->getSize();
		if (sz.height < ret.height)
			sz.height = ret.height;

		node->setPosition(x, y);

		sz.width += ret.width;
		x += ret.width;
	}

	return sz;
}
  
CJiraLinkNode::CJiraLinkNode(const std::string& href)
	: CJiraSpanNode(gui::elem::link)
{
	m_data[Attr::Href] = href;
}

CJiraTextNode::CJiraTextNode(const std::string& text)
	: CJiraNode(gui::elem::text)
{
	m_data[Attr::Text] = text;
}

void CJiraTextNode::paintContents(gui::painter* painter,
	const gui::pixels& offX, const gui::pixels& offY)
{
	gui::push_origin push{ painter };
	painter->moveOrigin({ offX, offY });
	painter->paintString(m_data[Attr::Text]);
}

gui::size CJiraTextNode::measureContents(gui::painter* painter,
	const gui::pixels&, const gui::pixels&)
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
	return std::make_shared<CJiraSpanNode>(gui::elem::span);
}

std::shared_ptr<gui::node> CJiraDocument::createSpan()
{
	return std::make_shared<CJiraSpanNode>(gui::elem::span);
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
	: CJiraBlockNode(gui::elem::table)
	, m_columns(std::make_shared<std::vector<gui::pixels>>())
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

gui::size CJiraTableNode::measureContents(gui::painter* painter,
	const gui::pixels& offX, const gui::pixels& offY)
{
	size_t columns = 0;
	for (auto& node : m_children) {
		auto values = node->children().size();
		if (columns < values)
			columns = values;
	}

	m_columns->assign(columns, 0);

	auto content = CJiraBlockNode::measureContents(painter, offX, offY);

	for (auto& node : m_children)
		static_cast<CJiraTableRowNode*>(node.get())->repositionChildren(painter);

	content.width = m_children.empty() ? 0 : m_children[0]->getSize().width;
	return content;
}

CJiraTableRowNode::CJiraTableRowNode(gui::elem name)
	: CJiraNode(name)
{
}

void CJiraTableRowNode::setColumns(const std::shared_ptr<std::vector<gui::pixels>>& columns)
{
	m_columns = columns;
}

gui::size CJiraTableRowNode::measureContents(gui::painter* painter,
	const gui::pixels& offX, const gui::pixels& offY)
{
	if (!m_columns)
		return{ 0, 0 };

	auto it = m_columns->begin();

	gui::size sz;
	auto x = offX;
	auto y = offY;
	for (auto& node : m_children) {
		node->measure(painter);
		auto ret = node->getSize();
		if (sz.height < ret.height)
			sz.height = ret.height;

		node->setPosition(x, y);

		sz.width += ret.width;
		x += ret.width;

		if (*it < ret.width)
			*it = ret.width;
		++it;
	}

	return sz;
}

void CJiraTableRowNode::repositionChildren(gui::painter* /*painter*/)
{
	auto it = m_columns->begin();

	gui::size sz;
	auto x = offsetLeft() + gui::pixels{ CELL_MARGIN };
	auto y = offsetTop();
	for (auto& node : m_children) {
		node->setPosition(x, y);
		x += *it++ + gui::pixels{ 2 * CELL_MARGIN };
	}

	m_position.size.width = x + gui::pixels{ CELL_MARGIN } + offsetRight();
}

CJiraReportElement::CJiraReportElement(const std::shared_ptr<jira::report>& dataset)
	: CJiraBlockNode(gui::elem::block)
	, m_dataset(dataset)
{
}

void CJiraReportElement::addChildren(const jira::server& server)
{
	{
		auto text = server.login() + "@" + server.displayName();
		auto title = std::make_shared<CJiraTextNode>(text);
		auto block = std::make_shared<CJiraSpanNode>(gui::elem::header);
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

				auto th = std::make_shared<CJiraSpanNode>(gui::elem::th);
				th->addChild(node);
				header->addChild(th);
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

CJiraDocumentElement::CJiraDocumentElement(const std::function<void(const gui::point&, const gui::size&)>& invalidator)
	: CJiraBlockNode(gui::elem::body)
	, m_invalidator(invalidator)
{
}

void CJiraDocumentElement::invalidate(const gui::point& pt, const gui::size& size)
{
	auto p = pt + m_position.pt;
	if (m_invalidator)
		m_invalidator(pt, size);
}
