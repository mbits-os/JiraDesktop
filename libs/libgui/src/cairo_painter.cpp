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
#include <gui/cairo_painter.hpp>

#if defined(HAS_CAIRO)

#pragma comment (lib, "cairo.lib")

#include <gui/image.hpp>
#include <gui/node.hpp>
#include <net/utf8.hpp>

#include <assert.h>
#define ASSERT(x) assert(x)

namespace gui { namespace cairo {

	template <size_t Index> static float channel(colorref clr)
	{
		auto ch = (clr >> (8 * Index)) & 0xFF;
		return (float)(ch / 255.0);
	}

	painter::painter(cairo_surface_t* surface, ratio zoom, const pixels& fontSize, const std::string& fontFamily)
		: base::painter{ zoom, fontSize, fontFamily }
		, m_surface{ surface }
		, m_cr{ cairo_create(surface) }
		, m_color(0)
	{
		selectFont(fontSize, fontFamily, 400, false, false);
	}

	painter::~painter()
	{
		cairo_destroy(m_cr);
	}

	void painter::paintImage(const image_ref* /*img*/, const pixels& /*width*/, const pixels& /*height*/)
	{
	}

	void painter::paintString(const std::string& text)
	{
		if (text.empty())
			return;

		cairo_set_source_rgb(m_cr, channel<0>(m_color), channel<1>(m_color), channel<2>(m_color));

		cairo_text_extents_t te;
		cairo_text_extents(m_cr, text.c_str(), &te);
		cairo_move_to(m_cr, zoom().scaleF(origin().x) - te.x_bearing, zoom().scaleF(origin().y) - te.y_bearing);
		cairo_show_text(m_cr, text.c_str());
	}

	size painter::measureString(const std::string& text)
	{
		cairo_text_extents_t te;
		cairo_text_extents(m_cr, text.c_str(), &te);
		return{ zoom().invert(te.width), zoom().invert(te.height) };
	}

	template<typename T>
	static inline T xp2dev(const ratio& r, T value)
	{
		return r.num * value / r.denom;
	}

	struct RectF {
		point pt;
		size sz;
	};

	static void FillSolidRect(cairo_t* cr, const RectF& rect, const ratio& zoom, colorref clr)
	{
		cairo_set_source_rgb(cr, channel<0>(clr), channel<1>(clr), channel<2>(clr));
		cairo_rectangle(cr, zoom.scaleD(rect.pt.x), zoom.scaleD(rect.pt.y), zoom.scaleD(rect.sz.width), zoom.scaleD(rect.sz.height));
		cairo_fill(cr);
	}

	void painter::fillRectangle(colorref color, const point& pt, const size& size)
	{
		FillSolidRect(m_cr, { origin() + pt, size }, zoom(), color);
	}

	void painter::drawBorder(line_style /*style*/, colorref color, const gui::point& pt, const gui::size& size)
	{
		FillSolidRect(m_cr, { origin() + pt, size }, zoom(), color);
	}

	static int gui2raw(weight w)
	{
		ASSERT(w != weight::bolder);
		ASSERT(w != weight::lighter);
		switch (w) {
		case weight::normal: return 400;
		case weight::bold: return 700;
		default:
			break;
		}

		return (int)w;
	}

	void painter::setFont(const pixels& fontSize, const std::string& fontFamily, gui::weight weight, bool italic, bool underline)
	{
		selectFont(fontSize, fontFamily, gui2raw(weight), italic, underline);
	}

	void painter::setColor(colorref color)
	{
		m_color = color;
	}

	colorref painter::getColor() const
	{
		return m_color;
	}

	void painter::selectFont(const pixels& fontSize, const std::string& fontFamily, int weight, bool italic, bool /*underline*/)
	{
		cairo_select_font_face(m_cr, fontFamily.c_str(),
			italic ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL,
			weight > 650 ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);

		cairo_set_font_size(m_cr, zoom().scaleD(fontSize));
	}
}};

#endif // defined(HAS_CAIRO)
