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

#include <gui/nodes/table_node.hpp>
#include <gui/nodes/row_node.hpp>

#include <assert.h>
#define ASSERT(x) assert(x)

namespace gui {
	table_node::table_node()
		: block_node(elem::table)
		, m_columns(std::make_shared<std::vector<pixels>>())
	{
	}

	table_node::table_node(const table_node&) = default;

	size table_node::measureContents(painter* painter)
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

		auto content = block_node::measureContents(painter);

		for (auto& node : m_children)
			static_cast<row_node*>(node.get())->gatherColumns();

		for (auto& node : m_children)
			static_cast<row_node*>(node.get())->repositionChildren();

		content.width = m_children.empty() ? 0 : m_children[0]->getSize().width;
		return content;
	}

	bool table_node::isSupported(const std::shared_ptr<node>& node)
	{
		auto elem = node->getNodeName();
		switch (elem) {
		case elem::table_caption:
		case elem::table_row:
		case elem::table_head:
			return true;
		default:
			break;
		}
		return false;
	}

	void table_node::onAdded(const std::shared_ptr<node>& node)
	{
		std::static_pointer_cast<row_node>(node)->setColumns(m_columns);
	}

	void table_node::onRemoved(const std::shared_ptr<node>& node)
	{
		std::static_pointer_cast<row_node>(node)->setColumns({});
	}

	std::shared_ptr<node> table_node::cloneSelf() const
	{
		return cloneDetach(std::make_shared<table_node>(*this));
	}
}
