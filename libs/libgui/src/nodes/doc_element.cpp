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

#include <gui/nodes/doc_element.hpp>

#include <assert.h>
#define ASSERT(x) assert(x)

namespace gui {
	doc_element::doc_element(const std::shared_ptr<doc_owner>& owner)
		: block_node(elem::body)
		, m_owner(owner)
	{
	}

	doc_element::doc_element(const doc_element&) = default;

	void doc_element::invalidate(const point& pt, const size& size)
	{
		auto p = pt + m_box.origin + m_content.origin;
		if (m_owner)
			m_owner->invalidate(p, size);
	}

	void doc_element::layoutRequired()
	{
		block_node::layoutRequired();
		if (m_owner)
			m_owner->layoutRequired();
	}

	std::shared_ptr<node> doc_element::cloneSelf() const
	{
		return cloneDetach(std::make_shared<doc_element>(*this));
	}
}
