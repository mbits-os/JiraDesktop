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

#include <gui/nodes/text_node.hpp>

#include <assert.h>
#define ASSERT(x) assert(x)

namespace gui {
	text_node::text_node(const std::string& text)
		: node_base(elem::text)
	{
		m_data[Attr::Text] = text;
	}

	text_node::text_node(const text_node&) = default;

	void text_node::innerText(const std::string& text)
	{
		m_data[Attr::Text] = text;
	}

	void text_node::paintContents(painter* painter)
	{
		painter->paintString(m_data[Attr::Text]);
	}

	size text_node::measureContents(painter* painter)
	{
		m_contentBaseline = painter->fontBaseline();
		return painter->measureString(m_data[Attr::Text]);
	}

	bool text_node::isSupported(const std::shared_ptr<node>&)
	{
		return false;
	}

	std::shared_ptr<node> text_node::cloneSelf() const
	{
		return cloneDetach(std::make_shared<text_node>(*this));
	}
}
