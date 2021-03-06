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

#include <gui/nodes/row_node.hpp>

#include <assert.h>
#define ASSERT(x) assert(x)

namespace gui {
	row_node::row_node(elem name)
		: span_node(name)
	{
	}

	row_node::row_node(const row_node&) = default;

	void row_node::setColumns(const std::shared_ptr<std::vector<pixels>>& columns)
	{
		m_columns = columns;
	}

	void row_node::gatherColumns()
	{
		if (!m_columns)
			return;

		auto it = m_columns->begin();

		for (auto& node : m_children) {
			auto ret = node->getContentSize();
			ret.width += node->offsetLeft() + node->offsetRight();
			if (*it < ret.width)
				*it = ret.width;
			++it;
		}
	}

	void row_node::repositionChildren()
	{
		auto it = m_columns->begin();

		size sz;
		pixels x = 0;
		pixels y = 0;
		auto h = m_box.size.height;
		h -= offsetTop() + offsetBottom();
		for (auto& node : m_children) {
			auto w = *it++;
			node->setPosition(x, y);
			node->setSize(w, h);
			x += w;
		}

		m_box.size.width = x + offsetLeft() + offsetRight();
	}

	std::shared_ptr<node> row_node::cloneSelf() const
	{
		return cloneDetach(std::make_shared<row_node>(*this));
	}
}
