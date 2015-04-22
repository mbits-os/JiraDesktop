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

#include <gui/nodes/caption_row_node.hpp>

#include <assert.h>
#define ASSERT(x) assert(x)

namespace gui {
	caption_row_node::caption_row_node()
		: row_node(elem::table_caption)
	{
	}

	caption_row_node::caption_row_node(const caption_row_node&) = default;

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
			internalSetSize(x, m_position.size.height);
	}

	std::shared_ptr<node> caption_row_node::cloneSelf() const
	{
		return cloneDetach(std::make_shared<caption_row_node>(*this));
	}
}
