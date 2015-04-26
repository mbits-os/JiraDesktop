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

#include <gui/nodes/span_node.hpp>

#include <assert.h>
#define ASSERT(x) assert(x)

namespace gui {
	span_node::span_node(elem name)
		: node_base(name)
	{
	}

	span_node::span_node(const span_node&) = default;

	size span_node::measureContents(painter* painter)
	{
		size sz;
		pixels x = 0;
		pixels y = 0;
		m_contentBaseline = 0;

		for (auto& node : m_children) {
			node->measure(painter);
			auto baseline = node->getNodeBaseline();
			if (baseline > m_contentBaseline)
				m_contentBaseline = baseline;
		}

		for (auto& node : m_children) {
			auto baseOffset = m_contentBaseline - node->getNodeBaseline();
			auto ret = node->getSize();
			auto reach = node->getReach();
			auto height = ret.height + baseOffset;
			if (sz.height < height)
				sz.height = height;

			node->setPosition(x + reach.left, y + baseOffset);

			auto width = ret.width + reach.left + reach.right;
			sz.width += width;
			x += width;
		}

		m_contentBaseline += m_content.origin.y;
		return sz;
	}

	std::shared_ptr<node> span_node::cloneSelf() const
	{
		return cloneDetach(std::make_shared<span_node>(*this));
	}
}
