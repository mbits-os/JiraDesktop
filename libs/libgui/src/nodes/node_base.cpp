/*
 * Copyright (C) 2015 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pch.h"

#include <gui/nodes/node_base.hpp>
#include <gui/nodes/text_node.hpp>

#include <assert.h>
#define ASSERT(x) assert(x)

namespace gui {
	node_base::node_base(elem name)
		: m_nodeName(name)
	{
	}

	node_base::node_base(const node_base& oth)
		: m_nodeName{ oth.m_nodeName }
		, m_data{ oth.m_data }
		, m_classes{ oth.m_classes }
		, m_parent{}
	{
	}

	const std::string& node_base::getId() const
	{
		auto id = m_data.find(Attr::Id);
		if (id != m_data.end())
			return id->second;

		static std::string dummy;
		return dummy;
	}

	void node_base::setId(const std::string& id)
	{
		m_data[Attr::Id] = id;
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

	std::shared_ptr<node> node_base::insertBefore(const std::shared_ptr<node>& newChild, const std::shared_ptr<node>& refChild)
	{
		if (!refChild)
			return appendChild(newChild);

		if (!newChild)
			return nullptr;

		if (!isSupported(newChild))
			return nullptr; // instead of HIERARCHY_REQUEST_ERR

		if (imChildOf(newChild))
			return nullptr; // instead of HIERARCHY_REQUEST_ERR

		auto locked = newChild->getParent();
		if (locked)
			locked->removeChild(newChild);

		auto it = std::find(std::begin(m_children), std::end(m_children), refChild);
		if (it == std::end(m_children))
			return nullptr; // instead of NOT_FOUND_ERR

		newChild->setParent(shared_from_this());
		newChild->applyStyles(m_stylesheet);
		m_children.insert(it, newChild);

		onAdded(newChild);

		return newChild;
	}

	std::shared_ptr<node> node_base::replaceChild(const std::shared_ptr<node>& newChild, const std::shared_ptr<node>& oldChild)
	{
		if (!newChild)
			return removeChild(oldChild);

		if (imChildOf(newChild))
			return nullptr; // instead of HIERARCHY_REQUEST_ERR

		auto locked = newChild->getParent();
		if (locked)
			locked->removeChild(newChild);

		auto it = std::find(std::begin(m_children), std::end(m_children), oldChild);
		if (it == std::end(m_children))
			return nullptr; // instead of NOT_FOUND_ERR

		oldChild->setParent({});
		oldChild->applyStyles({});
		onRemoved(oldChild);

		newChild->setParent(shared_from_this());
		newChild->applyStyles(m_stylesheet);
		*it = newChild;

		onAdded(newChild);

		return oldChild;
	}

	std::shared_ptr<node> node_base::removeChild(const std::shared_ptr<node>& oldChild)
	{
		auto it = std::find(std::begin(m_children), std::end(m_children), oldChild);
		if (it == std::end(m_children))
			return nullptr; // instead of NOT_FOUND_ERR

		oldChild->setParent({});
		m_children.erase(it);

		layoutRequired();
		onRemoved(oldChild);

		return oldChild;
	}

	std::shared_ptr<node> node_base::appendChild(const std::shared_ptr<node>& newChild)
	{
		if (!newChild || !isSupported(newChild))
			return nullptr; // instead of HIERARCHY_REQUEST_ERR

		if (imChildOf(newChild))
			return nullptr; // instead of HIERARCHY_REQUEST_ERR

		auto locked = newChild->getParent();
		if (locked)
			locked->removeChild(newChild);

		newChild->setParent(shared_from_this());
		newChild->applyStyles(m_stylesheet);
		m_children.push_back(newChild);

		onAdded(newChild);

		return newChild;
	}

	void node_base::removeAllChildren()
	{
		std::vector<std::shared_ptr<node>> nodes;
		nodes.swap(m_children);

		for (auto& oldChild : nodes)
			oldChild->setParent({});

		layoutRequired();
		for (auto& oldChild : nodes)
			onRemoved(oldChild);
	}

	bool node_base::hasChildNodes() const
	{
		return !m_children.empty();
	}

	std::shared_ptr<node> node_base::cloneNode(bool deep) const
	{
		auto clone = cloneSelf();
		if (!clone || !deep)
			return clone;

		for (auto& child : m_children) {
			auto ch = child->cloneNode(true);
			if (ch)
				clone->appendChild(ch);
		}
		return clone;
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
		if (!style)
			return;

		auto& ref = *style;
		if (ref.has(styles::prop_background)) {
			painter->paintBackground(ref.get(styles::prop_background),
				m_box.size.width, m_box.size.height);
		}

		painter->paintBorder(this);

		painter->moveOrigin(m_content.origin);
		paintContents(painter);
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
		if (!m_layoutCount)
			return; // already calculated
		m_layoutCount = 0;

		calculateStyles();
		style_saver saver{ painter, this };

		point tl{ offsetLeft(), offsetTop() };
		point br{ offsetRight(), offsetBottom() };

		m_content.size = measureContents(painter);
		m_content.origin = tl;

		auto size = tl + br + m_content.size;

		internalSetSize(size.x, size.y);

		findContentReach();
	}

	void node_base::setPosition(const pixels& x, const pixels& y)
	{
		if (!m_box.size.empty())
			invalidate();
		m_box.origin = { x, y };
		if (!m_box.size.empty())
			invalidate();
	}

	void node_base::setSize(const pixels& width, const pixels& height)
	{
		internalSetSize(width, height);

		auto styles = calculatedStyle();
		if (!styles)
			return;

		auto& ref = *styles;

		auto disp = display::inlined;
		if (ref.has(styles::prop_display))
			disp = ref.get(styles::prop_display);

		if (disp == display::table_cell) {
			auto align = align::left;
			if (ref.has(styles::prop_text_align))
				align = ref.get(styles::prop_text_align);

			auto inside_padding_w = width - (offsetLeft() + offsetRight());
			auto inside_padding_h = height - (offsetTop() + offsetBottom());

			//new vector of the content
			m_content.origin = { 0, offsetTop() + (inside_padding_h - m_content.size.height) / 2 };

			if (align != align::left) {
				m_content.origin.x = inside_padding_w - m_content.size.width;

				if (align == align::center)
					m_content.origin.x = m_content.origin.x / 2;
			}
			m_content.origin.x += offsetLeft();

			updateReach();
		}
	}

	point node_base::getPosition()
	{
		return m_box.origin;
	}

	point node_base::getAbsolutePos()
	{
		auto parent = m_parent.lock();
		if (!parent)
			return getPosition();
		auto pt = parent->getAbsolutePos();
		auto off = parent->getContentPosition();
		return pt + off + m_box.origin;
	}

	size node_base::getSize()
	{
		return m_box.size;
	}

	box node_base::getMargin()
	{
		return m_styled.margin;
	}

	box node_base::getBorder()
	{
		return m_styled.border;
	}

	box node_base::getPadding()
	{
		return m_styled.padding;
	}

	box node_base::getReach()
	{
		return m_box.values;
	}

	point node_base::getContentPosition()
	{
		return m_content.origin;
	}

	size node_base::getContentSize()
	{
		return m_content.size;
	}

	box node_base::getContentReach()
	{
		return m_content.values;
	}

	pixels node_base::getContentBaseline()
	{
		return m_contentBaseline;
	}

	pixels node_base::getNodeBaseline()
	{
		return m_contentBaseline + m_content.origin.y;
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
		invalidate(-m_content.origin, m_box.size);
	}

	void node_base::invalidate(const point& pt, const size& size)
	{
		auto p = pt + m_box.origin + m_content.origin;
		auto parent = m_parent.lock();
		if (parent)
			parent->invalidate(p, size);
	}

	std::shared_ptr<node> node_base::nodeFromPoint(const pixels& x_, const pixels& y_)
	{
		auto x = x_ - m_box.origin.x;
		auto y = y_ - m_box.origin.y;

		if (x < 0 || x > m_box.size.width ||
			y < 0 || y > m_box.size.height)
			return nullptr;

		x -= m_content.origin.x;
		y -= m_content.origin.y;
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

	void node_base::paintContents(painter* painter)
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
		appendChild(std::make_shared<text_node>(text));
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
		m_allApplying = std::make_shared<styles::stylesheet>();
		m_stylesheet = stylesheet;
		if (stylesheet) {
			for (auto& rules : stylesheet->m_rules) {
				if (rules->m_sel.maySelect(this))
					m_allApplying->m_rules.push_back(rules);
			}
		}

		for (auto& node : children())
			node->applyStyles(stylesheet);

		layoutRequired();
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

		updateBoxes();
	}

	static void setBox(styles::rule_storage& ref, gui::box& box,
		styles::length_prop top, styles::length_prop right,
		styles::length_prop bottom, styles::length_prop left)
	{
		box.top = calculated(ref, top);
		box.right = calculated(ref, right);
		box.bottom = calculated(ref, bottom);
		box.left = calculated(ref, left);
	}

	void node_base::updateBoxes()
	{
		m_styled = {};
		auto style = calculatedStyle();
		if (!style)
			return;

		auto& ref = *style;
		using namespace styles;

		setBox(ref, m_styled.margin, prop_margin_top, prop_margin_right, prop_margin_bottom, prop_margin_left);
		setBox(ref, m_styled.border, prop_border_top_width, prop_border_right_width, prop_border_bottom_width, prop_border_left_width);
		setBox(ref, m_styled.padding, prop_padding_top, prop_padding_right, prop_padding_bottom, prop_padding_left);

		updateReach();
	}

	void node_base::findContentReach()
	{
		pixels before, after;
		if (!m_children.empty()) {
			auto& child = m_children[0];
			auto reach = child->getReach();
			auto height = child->getSize().height;
			auto y = child->getPosition().y;
			before = y - reach.top;
			after = y + height + reach.bottom;
		}

		for (auto& child : m_children) {
			auto reach = child->getReach();
			auto height = child->getSize().height;
			auto y = child->getPosition().y;

			auto B = y - reach.top;
			auto A = y + height + reach.bottom;

			if (before > B)
				before = B;
			if (after < A)
				after = A;
		}

		m_content.values = { m_content.origin.y - before, 0, after - (m_content.origin.y + m_content.size.height) };
		updateReach();
	}

	void node_base::updateReach()
	{
		m_box.values.left = m_styled.margin.left;
		m_box.values.right = m_styled.margin.right;
		m_box.values.top = std::max(m_styled.margin.top, m_content.values.top - m_content.origin.y);
		auto content_after = m_box.size.height - m_content.origin.y - m_content.size.height;
		m_box.values.bottom = std::max(m_styled.margin.bottom, m_content.values.bottom - content_after);
	}

	pixels node_base::offsetLeft() const
	{
		return m_styled.border.left + m_styled.padding.left;
	}

	pixels node_base::offsetTop() const
	{
		return m_styled.border.top + m_styled.padding.top;
	}

	pixels node_base::offsetRight() const
	{
		return m_styled.border.right + m_styled.padding.right;
	}

	pixels node_base::offsetBottom() const
	{
		return m_styled.border.bottom + m_styled.padding.bottom;
	}

	bool node_base::isTabStop() const
	{
		return false;
	}

	std::shared_ptr<node> node_base::getNextItem(bool freshLookup) const
	{
		auto here = const_cast<node_base*>(this)->shared_from_this();
		auto start = freshLookup ? nullptr : here;

		while (here) {
			auto ptr = static_cast<node_base*>(here.get())->nextTabStop(start);
			if (ptr) return ptr;

			start = here;
			here = here->getParent();
		}

		return{};
	}

	std::shared_ptr<node> node_base::getPrevItem(bool freshLookup) const
	{
		auto here = const_cast<node_base*>(this)->shared_from_this();
		auto start = freshLookup ? nullptr : here;

		while (here) {
			auto ptr = static_cast<node_base*>(here.get())->prevTabStop(start);
			if (ptr) return ptr;

			start = here;
			here = here->getParent();
		}

		return{};
	}

	std::shared_ptr<node> node_base::nextTabStop(const std::shared_ptr<node>& start) const
	{
		if (!start && isTabStop())
			return const_cast<node_base*>(this)->shared_from_this(); // const...

		auto begin = std::begin(m_children);
		auto end = std::end(m_children);
		auto restart = start && start.get() != this;

		auto it = restart ? std::find(begin, end, start) : begin;

		if (restart) {
			if (it == end)
				return{};

			++it;
		}

		for (; it != end; ++it) {
			auto ret = static_cast<node_base*>(it->get())->nextTabStop({});
			if (ret)
				return ret;
		}

		return{};
	}

	std::shared_ptr<node> node_base::prevTabStop(const std::shared_ptr<node>& start) const
	{
		if (start.get() == this)
			return{}; // this node was visited as lats one in this subtree, moving up

		auto begin = std::rbegin(m_children);
		auto end = std::rend(m_children);
		auto restart = !!start;

		auto it = restart ? std::find(begin, end, start) : begin;

		if (restart) {
			if (it == end)
				return{};

			++it;
		}

		for (; it != end; ++it) {
			auto ret = static_cast<node_base*>(it->get())->prevTabStop({});
			if (ret)
				return ret;
		}

		if (isTabStop())
			return const_cast<node_base*>(this)->shared_from_this();

		return{};
	}

	bool node_base::isSupported(const std::shared_ptr<node>&)
	{
		auto it = m_data.find(Attr::Text);
		return it == m_data.end();
	}

	void node_base::onAdded(const std::shared_ptr<node>&)
	{
	}

	void node_base::onRemoved(const std::shared_ptr<node>&)
	{
	}

	void node_base::layoutRequired()
	{
		m_calculated.reset();
		m_calculatedHover.reset();
		m_calculatedActive.reset();
		m_calculatedHoverActive.reset();

		++m_layoutCount;
		auto parent = getParent();
		if (parent)
			static_cast<node_base&>(*parent).layoutRequired();
	}

	void node_base::internalSetSize(const pixels& width, const pixels& height)
	{
		if (!m_box.size.empty())
			invalidate();

		m_box.size = { width, height };

		if (!m_box.size.empty())
			invalidate();
	}

	bool node_base::imChildOf(const std::shared_ptr<node>& tested) const
	{
		auto parent = m_parent.lock();
		while (parent) {
			if (parent == tested)
				return true;

			parent = parent->getParent();
		}

		return false;
	}
}
