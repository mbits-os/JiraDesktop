#include "pch.h"

#ifdef max
#undef max
#endif

#include <gui/nodes.hpp>
#include <gui/painter.hpp>
#include <gui/node.hpp>
#include <limits>
#include <sstream>

#include <net/utf8.hpp>

#include <assert.h>
#define ASSERT(x) assert(x)

namespace gui {
	using namespace styles::literals;

	node_base::node_base(elem name)
		: m_nodeName(name)
	{
	}

	elem node_base::getNodeName() const
	{
		return m_nodeName;
	}

	void node_base::addClass(const std::string& name)
	{
		auto it = std::find(std::begin(m_classes), std::end(m_classes), name);
		if (it == std::end(m_classes))
			m_classes.push_back(name);
	}

	void node_base::removeClass(const std::string& name)
	{
		auto it = std::find(std::begin(m_classes), std::end(m_classes), name);
		if (it != std::end(m_classes))
			m_classes.erase(it);
	}

	bool node_base::hasClass(const std::string& name) const
	{
		auto it = std::find(std::begin(m_classes), std::end(m_classes), name);
		return it != std::end(m_classes);
	}

	const std::vector<std::string>& node_base::getClassNames() const
	{
		return m_classes;
	}

	std::string node_base::text() const
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

	void node_base::setTooltip(const std::string& text)
	{
		m_data[Attr::Tooltip] = text;
	}

	void node_base::addChild(const std::shared_ptr<node>& child)
	{
		auto it = m_data.find(Attr::Text);
		if (it != m_data.end())
			return;

		child->setParent(shared_from_this());
		m_children.push_back(std::move(child));
	}

	const std::vector<std::shared_ptr<node>>& node_base::children() const
	{
		return m_children;
	}

	class style_saver {
		painter* m_painter;
		style_handle m_handle;
	public:
		style_saver(gui::painter* painter, gui::node* node) : m_painter(painter), m_handle(nullptr)
		{
			if (!node)
				return;

			m_handle = m_painter->applyStyle(node);
		}

		~style_saver()
		{
			m_painter->restoreStyle(m_handle);
		}
	};

	void node_base::paint(painter* painter)
	{
		if (!painter->visible(this))
			return;

		style_saver saver{ painter, this };

		auto style = calculatedStyle();
		auto& ref = *style;
		if (ref.has(styles::prop_background)) {
			painter->paintBackground(ref.get(styles::prop_background),
				m_position.size.width, m_position.size.height);
		}

		painter->paintBorder(this);

		paintContents(painter, offsetLeft(), offsetTop());
	}

	static pixels calculated(const styles::rule_storage& rules,
		styles::length_prop prop)
	{
		if (!rules.has(prop))
			return 0.0;

		auto u = rules.get(prop);
		ASSERT(u.which() == styles::length_u::first_type);

		return u.first();
	};

	void node_base::measure(painter* painter)
	{
		calculateStyles();
		style_saver saver{ painter, this };

		point tl{ offsetLeft(), offsetTop() };
		point br{ offsetRight(), offsetBottom() };
		auto size = measureContents(painter, tl.x, tl.y);

		m_position.size = { tl.x + br.x + size.width, tl.y + br.y + size.height };
	}

	void node_base::setPosition(const pixels& x, const pixels& y)
	{
		m_position.pt = { x, y };
	}

	void node_base::setSize(const pixels& width, const pixels& height)
	{
		m_position.size = { width, height };

		auto styles = calculatedStyle();
		auto& ref = *styles;

		auto disp = display::inlined;
		if (ref.has(styles::prop_display))
			disp = ref.get(styles::prop_display);

		if (disp == display::table_cell) {

			auto align = align::left;
			if (ref.has(styles::prop_text_align))
				align = ref.get(styles::prop_text_align);

			pixels w;
			if (align != align::left) {
				for (auto& child : m_children)
					w += child->getSize().width;

				w = (width - (offsetLeft() + offsetRight())) - w;

				if (align == align::center)
					w = w.value() / 2;
			}

			auto h = m_position.size.height;
			h -= offsetBottom() + offsetTop();

			for (auto& child : m_children) {
				auto pos = child->getPosition();
				auto h_ = child->getSize().height;
				child->setPosition(pos.x + w, pos.y +
					pixels{ (h - h_).value() / 2 });
			}
		}
	}

	point node_base::getPosition()
	{
		return m_position.pt;
	}

	point node_base::getAbsolutePos()
	{
		auto parent = m_parent.lock();
		if (!parent)
			return getPosition();
		auto pt = parent->getAbsolutePos();
		return pt + m_position.pt;
	}

	size node_base::getSize()
	{
		return m_position.size;
	}

	std::shared_ptr<node> node_base::getParent() const
	{
		return m_parent.lock();
	}

	void node_base::setParent(const std::shared_ptr<node>& node)
	{
		m_parent = node;
	}

	void node_base::invalidate()
	{
		invalidate({ 0, 0 }, m_position.size);
	}

	void node_base::invalidate(const point& pt, const size& size)
	{
		auto p = pt + m_position.pt;
		auto parent = m_parent.lock();
		if (parent)
			parent->invalidate(p, size);
	}

	std::shared_ptr<node> node_base::nodeFromPoint(const pixels& x_, const pixels& y_)
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

	void node_base::setHovered(bool hovered)
	{
		bool changed = false;
		if (hovered) {
			auto value = ++m_hoverCount;
			changed = value == 1;
		}
		else {
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

	bool node_base::getHovered() const
	{
		return m_hoverCount > 0;
	}

	void node_base::setActive(bool active)
	{
		bool changed = false;
		if (active) {
			auto value = ++m_activeCount;
			changed = value == 1;
		}
		else {
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

	bool node_base::getActive() const
	{
		return m_activeCount > 0;
	}

	void node_base::activate()
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

	void node_base::paintContents(painter* painter,
		const pixels&, const pixels&)
	{
		for (auto& node : m_children) {
			push_origin push{ painter };
			painter->moveOrigin(node->getPosition());
			node->paint(painter);
		}
	}

	pointer node_base::getCursor() const
	{
		auto styles = calculatedStyle();
		if (styles) {
			if (styles->has(styles::prop_cursor)) {
				auto c = styles->get(styles::prop_cursor);
				if (c != pointer::inherited)
					return c;
			}
		}

		auto parent = m_parent.lock();
		if (parent)
			return parent->getCursor();

		return pointer::inherited;
	}

	bool node_base::hasTooltip() const
	{
		auto it = m_data.find(Attr::Tooltip);
		return it != m_data.end();
	}

	const std::string& node_base::getTooltip() const
	{
		auto it = m_data.find(Attr::Tooltip);
		if (it != m_data.end())
			return it->second;

		static std::string dummy;
		return dummy;
	}

	void node_base::innerText(const std::string& text)
	{
		m_children.clear();
		addChild(std::make_shared<CJiraTextNode>(text));
	}

	std::shared_ptr<styles::rule_storage> node_base::calculatedStyle() const
	{
		if (getHovered()) {
			return getActive() ? m_calculatedHoverActive : m_calculatedHover;
		}

		return getActive() ? m_calculatedActive : m_calculated;
	}

	std::shared_ptr<styles::rule_storage> node_base::normalCalculatedStyles() const
	{
		return m_calculated;
	}

	std::shared_ptr<styles::stylesheet> node_base::styles() const
	{
		return m_allApplying;
	}

	void node_base::applyStyles(const std::shared_ptr<styles::stylesheet>& stylesheet)
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

	pixels calculatedFontSize(node* node)
	{
		using namespace styles;
		using namespace literals;

		if (!node)
			return 14_px;

		auto styles = node->normalCalculatedStyles();
		if (styles && styles->has(prop_font_size)) {
			auto u = styles->get(prop_font_size);
			ASSERT(u.which() == length_u::first_type);
			return u.first();
		}

		return calculatedFontSize(node->getParent().get());
	}

	pixels parentFontSize(node* node)
	{
		return calculatedFontSize(node->getParent().get());
	}

	pixels calculate(styles::rule_storage& rules,
		styles::length_prop prop,
		const pixels& px)
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

	void calculate(styles::rule_storage& rules, node* node)
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

	void node_base::calculateStyles()
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
		}
		else {
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

	pixels node_base::offsetLeft() const
	{
		auto style = calculatedStyle();
		auto& ref = *style;
		return
			calculated(ref, styles::prop_border_left_width) +
			calculated(ref, styles::prop_padding_left);
	}

	pixels node_base::offsetTop() const
	{
		auto style = calculatedStyle();
		auto& ref = *style;
		return
			calculated(ref, styles::prop_border_top_width) +
			calculated(ref, styles::prop_padding_top);
	}

	pixels node_base::offsetRight() const
	{
		auto style = calculatedStyle();
		auto& ref = *style;
		return
			calculated(ref, styles::prop_border_right_width) +
			calculated(ref, styles::prop_padding_right);
	}

	pixels node_base::offsetBottom() const
	{
		auto style = calculatedStyle();
		auto& ref = *style;
		return
			calculated(ref, styles::prop_border_bottom_width) +
			calculated(ref, styles::prop_padding_bottom);
	}

	void image_cb::onImageChange(image_ref*)
	{
		auto par = parent.lock();
		if (par)
			par->invalidate();
	}

	icon_node::icon_node(const std::string& uri, const std::shared_ptr<image_ref>& image, const std::string& tooltip)
		: node_base(elem::image)
		, m_image(image)
		, m_cb(std::make_shared<image_cb>())
	{
		m_data[Attr::Href] = uri;
		node_base::setTooltip(tooltip);
		m_position.size.width = m_position.size.height = 16_px;
	}

	icon_node::~icon_node()
	{
		m_image->unregisterListener(m_cb);
		m_cb->parent.reset();
	}

	void icon_node::attach()
	{
		m_cb->parent = shared_from_this();
		m_image->registerListener(m_cb);
	}

	void icon_node::addChild(const std::shared_ptr<node>& /*child*/)
	{
		// noop
	}

	void icon_node::paintContents(painter* painter,
		const pixels& offX, const pixels& offY)
	{
		push_origin push{ painter };
		painter->moveOrigin({ offX, offY });
		painter->paintImage(m_image.get(), 16_px, 16_px);
	}

	size icon_node::measureContents(painter*,
		const pixels&, const pixels&)
	{
		return{ 16_px, 16_px };
	}

	user_node::user_node(const std::weak_ptr<document_base>& document, std::map<uint32_t, std::string>&& avatar, const std::string& tooltip)
		: node_base(elem::icon)
		, m_document(document)
		, m_cb(std::make_shared<image_cb>())
		, m_urls(std::move(avatar))
		, m_selectedSize(0)
	{
		node_base::setTooltip(tooltip);
		m_position.size.width = m_position.size.height = 16_px;
	}

	user_node::~user_node()
	{
		m_image->unregisterListener(m_cb);
		m_cb->parent.reset();
	}

	void user_node::addChild(const std::shared_ptr<node>& /*child*/)
	{
		// noop
	}

	void user_node::paintContents(painter* painter,
		const pixels& offX, const pixels& offY)
	{
		push_origin push{ painter };
		painter->moveOrigin({ offX, offY });
		painter->paintImage(m_image.get(), 16_px, 16_px);
	}

	size user_node::measureContents(painter* painter,
		const pixels&, const pixels&)
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
		}
		else {
			size_t delta = std::numeric_limits<size_t>::max();
			for (auto& pair : m_urls) {
				if (pair.first < scaled)
					continue;

				size_t d = pair.first - scaled;
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

	block_node::block_node(elem name)
		: node_base(name)
	{
	}

	size block_node::measureContents(painter* painter,
		const pixels& offX, const pixels& offY)
	{
		size sz;
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

	span_node::span_node(elem name)
		: node_base(name)
	{
	}

	size span_node::measureContents(painter* painter,
		const pixels& offX, const pixels& offY)
	{
		size sz;
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
		: span_node(elem::link)
	{
		m_data[Attr::Href] = href;
	}

	CJiraTextNode::CJiraTextNode(const std::string& text)
		: node_base(elem::text)
	{
		m_data[Attr::Text] = text;
	}

	void CJiraTextNode::paintContents(painter* painter,
		const pixels& offX, const pixels& offY)
	{
		push_origin push{ painter };
		painter->moveOrigin({ offX, offY });
		painter->paintString(m_data[Attr::Text]);
	}

	size CJiraTextNode::measureContents(painter* painter,
		const pixels&, const pixels&)
	{
		return painter->measureString(m_data[Attr::Text]);
	}

	std::shared_ptr<document> document::make_doc(const std::shared_ptr<image_creator>& creator)
	{
		return std::make_shared<document_base>(creator);
	}

	document_base::document_base(const std::shared_ptr<image_creator>& creator)
		: m_creator(creator)
	{
	}

	std::shared_ptr<node> document_base::createIcon(const std::string& uri, const std::string& text, const std::string& description)
	{
		auto image = createImage(uri);
		auto tooltip = text;
		if (text.empty() || description.empty())
			tooltip += description;
		else
			tooltip += "\n" + description;

		auto icon = std::make_shared<icon_node>(uri, image, tooltip);
		icon->attach();
		return icon;
	}

	std::shared_ptr<node> document_base::createUser(bool /*active*/, const std::string& display, const std::string& email, const std::string& /*login*/, std::map<uint32_t, std::string>&& avatar)
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

		return std::make_shared<user_node>(shared_from_this(), std::move(avatar), tooltip);
	}

	std::shared_ptr<node> document_base::createLink(const std::string& href)
	{
		return std::make_shared<CJiraLinkNode>(href);
	}

	std::shared_ptr<node> document_base::createText(const std::string& text)
	{
		return std::make_shared<CJiraTextNode>(text);
	}

	std::shared_ptr<image_ref> document_base::createImage(const std::string& uri)
	{
		std::lock_guard<std::mutex> lock(m_guard);
		auto it = m_cache.lower_bound(uri);
		if (it != m_cache.end() && it->first == uri)
			return it->second;

		ASSERT(!!m_creator);
		auto image = m_creator->create(uri);
		m_cache.insert(it, std::make_pair(uri, image));
		return image;
	}

	std::shared_ptr<node> document_base::createElement(const elem name)
	{
		switch (name) {
		case elem::block: return std::make_shared<block_node>(name);
		case elem::header: return std::make_shared<span_node>(name);
		case elem::span: return std::make_shared<span_node>(name);
		case elem::table: return std::make_shared<table_node>();
		case elem::table_caption: return std::make_shared<caption_row_node>();
		case elem::table_head: return std::make_shared<row_node>(name);
		case elem::table_row: return std::make_shared<row_node>(name);
		case elem::th: return std::make_shared<span_node>(name);
		case elem::td: return std::make_shared<span_node>(name);
		};

		return{};
	}

	table_node::table_node()
		: block_node(elem::table)
		, m_columns(std::make_shared<std::vector<pixels>>())
	{
	}

	void table_node::addChild(const std::shared_ptr<node>& child)
	{
		auto elem = child->getNodeName();
		switch (elem) {
		case elem::table_caption:
		case elem::table_row:
		case elem::table_head:
			node_base::addChild(child);
			std::static_pointer_cast<row_node>(child)->setColumns(m_columns);
		default:
			break;
		}
	}

	size table_node::measureContents(painter* painter,
		const pixels& offX, const pixels& offY)
	{
		size_t columns = 0;
		for (auto& node : m_children) {
			auto name = node->getNodeName();
			auto values =
				name == elem::table_caption ? 1 : node->children().size();
			if (columns < values)
				columns = values;
		}

		m_columns->assign(columns, 0);

		auto content = block_node::measureContents(painter, offX, offY);

		for (auto& node : m_children)
			static_cast<row_node*>(node.get())->repositionChildren();

		content.width = m_children.empty() ? 0 : m_children[0]->getSize().width;
		return content;
	}

	row_node::row_node(elem name)
		: span_node(name)
	{
	}

	void row_node::setColumns(const std::shared_ptr<std::vector<pixels>>& columns)
	{
		m_columns = columns;
	}

	size row_node::measureContents(painter* painter,
		const pixels& offX, const pixels& offY)
	{
		if (!m_columns)
			return{ 0, 0 };

		auto it = m_columns->begin();

		size sz;
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

	void row_node::repositionChildren()
	{
		auto it = m_columns->begin();

		size sz;
		auto x = offsetLeft();
		auto y = offsetTop();
		auto h = m_position.size.height;
		h -= offsetTop() + offsetBottom();
		for (auto& node : m_children) {
			auto w = *it++;
			node->setPosition(x, y);
			node->setSize(w, h);
			x += w;
		}

		m_position.size.width = x + offsetRight();
	}

	caption_row_node::caption_row_node()
		: row_node(elem::table_caption)
	{
	}

	size caption_row_node::measureContents(painter* painter,
		const pixels& offX, const pixels& offY)
	{
		// Grandfather call, skip columns setting
		return span_node::measureContents(painter, offX, offY);
	}

	void caption_row_node::repositionChildren()
	{
		// make sure there is enough space after the last child...
		if (m_columns->empty())
			return;

		auto it = m_columns->begin();
		pixels x = 0;
		for (auto& w : *m_columns)
			x += w;

		if (x < m_position.size.width)
			m_columns->back() += m_position.size.width - x;
		else
			m_position.size.width = x;
	}

	doc_element::doc_element(const std::function<void(const point&, const size&)>& invalidator)
		: block_node(elem::body)
		, m_invalidator(invalidator)
	{
	}

	void doc_element::invalidate(const point& pt, const size& size)
	{
		auto p = pt + m_position.pt;
		if (m_invalidator)
			m_invalidator(pt, size);
	}
}