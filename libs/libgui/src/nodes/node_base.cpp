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
		onRemoved(oldChild);

		newChild->setParent(shared_from_this());
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
		m_children.push_back(newChild);

		onAdded(newChild);

		return newChild;
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

	pixels node_base::getBaseline()
	{
		return m_baseline;
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
