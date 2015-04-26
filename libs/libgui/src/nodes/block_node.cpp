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

#include <gui/nodes/block_node.hpp>

#include <assert.h>
#define ASSERT(x) assert(x)

namespace gui {
	block_node::block_node(elem name)
		: node_base(name)
	{
	}

	block_node::block_node(const block_node&) = default;

	size block_node::measureContents(painter* painter)
	{
		size sz;
		pixels x = 0;
		pixels y = 0;
		pixels previous = 0;
		bool first = 0;
		for (auto& node : m_children) {
			node->measure(painter);
			auto ret = node->getSize();
			auto reach = node->getReach();
			auto width = ret.width + reach.left + reach.right;
			if (sz.width < width)
				sz.width = width;

			pixels pre;
			if (first)
				first = true;
			else
				pre = max(previous, reach.top);

			node->setPosition(x + reach.left, y + pre);

			sz.height += ret.height + pre;
			y += ret.height + pre;

			previous = reach.bottom;
		}

		m_contentBaseline = 0;
		if (!m_children.empty())
			m_contentBaseline = m_children.front()->getNodeBaseline();

		return sz;
	}

	std::shared_ptr<node> block_node::cloneSelf() const
	{
		return cloneDetach(std::make_shared<block_node>(*this));
	}
}
