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
#include <gui/image.hpp>
#include <gui/node.hpp>
#include <net/utf8.hpp>

#include <assert.h>
#define ASSERT(x) assert(x)

namespace gui { namespace cairo {
	painter::painter(cairo_surface_t* surface, ratio zoom, ratio device, const pixels& fontSize, const std::string& fontFamily)
		: m_zoom{ 0, 0 }
		, m_device{ device.num, device.denom }
		, m_origin{ 0, 0 }
		, m_fontSize{ fontSize }
		, m_fontFamily{ fontFamily }
		, m_surface{ surface }
		, m_cr{ cairo_create(surface) }
	{
		m_zoom = zoom * m_device;
	}

	painter::~painter()
	{
		cairo_destroy(m_cr);
	}

	void painter::moveOrigin(const pixels& x, const pixels& y)
	{
		m_origin.x += x;
		m_origin.y += y;
	}

	point painter::getOrigin() const
	{
		return m_origin;
	}

	void painter::setOrigin(const point& orig)
	{
		m_origin = orig;
	}

	void painter::paintImage(const image_ref* /*img*/, const pixels& /*width*/, const pixels& /*height*/)
	{
	}

	void painter::paintString(const std::string& text)
	{
		if (text.empty())
			return;

		cairo_text_extents_t te;
		cairo_text_extents(m_cr, text.c_str(), &te);
		cairo_move_to(m_cr, m_zoom.scaleF(m_origin.x) + te.x_bearing, m_zoom.scaleF(m_origin.y) + te.y_bearing);
		cairo_show_text(m_cr, text.c_str());
	}

	size painter::measureString(const std::string& text)
	{
		cairo_text_extents_t te;
		cairo_text_extents(m_cr, text.c_str(), &te);
		return{ m_zoom.invert(te.width), m_zoom.invert(te.height) };
	}

	template<typename T>
	static inline T xp2dev(const ratio& r, T value)
	{
		return r.num * value / r.denom;
	}

	int painter::dpiRescale(int size)
	{
		return xp2dev(m_zoom, size);
	}

	long double painter::dpiRescale(long double size)
	{
		return xp2dev(m_zoom, size);
	}

	bool painter::visible(node* /*node*/) const
	{
		return true;
	}

	gui::style_handle painter::applyStyle(node* /*node*/)
	{
		return nullptr;
	}

	void painter::restoreStyle(gui::style_handle /*saved*/)
	{
	}
}};
