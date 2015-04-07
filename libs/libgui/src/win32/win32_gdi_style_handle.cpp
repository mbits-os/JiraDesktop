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
#include <gui/gdi_style_handle.hpp>
#include <gui/node.hpp>

#include <assert.h>
#define ASSERT(x) assert(x)

namespace gui { namespace gdi {
	static long double calculated(const styles::rule_storage& rules,
		gui::painter* painter, styles::length_prop prop)
	{
		if (!rules.has(prop))
			return 0.0;

		auto u = rules.get(prop);
		ASSERT(u.which() == styles::length_u::first_type);

		return painter->dpiRescale(u.first().value());
	};

	void style_save::apply(callback* cb, gui::node* node)
	{
		m_caller = cb;
		m_target = node;
		auto style = node->calculatedStyle();
		auto& ref = *style;

		// ...

		if (ref.has(styles::prop_background))
			m_caller->drawBackground(node, ref.get(styles::prop_background));

		m_caller->drawBorder(node);

		auto painter = m_caller->getPainter();
		m_origin = painter->getOrigin();

		auto padx = 0.5 +
			calculated(ref, painter, styles::prop_border_left_width) +
			calculated(ref, painter, styles::prop_padding_left);
		auto pady = 0.5 +
			calculated(ref, painter, styles::prop_border_top_width) +
			calculated(ref, painter, styles::prop_padding_top);

		painter->moveOrigin({ int(padx), int(pady) });
	}

	void style_save::restore()
	{
		m_caller->getPainter()->setOrigin(m_origin);
	}
}};
