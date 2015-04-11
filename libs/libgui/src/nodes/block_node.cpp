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
}
