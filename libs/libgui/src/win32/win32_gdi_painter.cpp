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
#include <gui/gdi_painter.hpp>
#include <gui/image.hpp>
#include <gui/node.hpp>
#include <net/utf8.hpp>

#include <assert.h>
#define ASSERT(x) assert(x)

#pragma warning(push)
 // warning C4458: declaration of 'name' hides class member
#pragma warning(disable: 4458)
#include <gdiplus.h>
#pragma warning(pop)

#pragma comment (lib, "gdiplus.lib")

namespace gui { namespace gdi {
	painter::painter(HDC dc, ratio zoom, const RECT& clip, HFONT font)
		: m_dc{ dc }
		, m_font{ font }
		, m_modified{ nullptr }
		, m_origin{ 0, 0 }
		, m_clip{ clip.left, clip.top, clip.right, clip.bottom }
	{
		m_device = ratio{ ::GetDeviceCaps(m_dc, LOGPIXELSX), 96 }.gcd();
		m_zoom = zoom * m_device;

		memset(&m_lf, 0, sizeof(m_lf));
		m_original = (HFONT)::SelectObject(m_dc, m_font);
		::GetObject(m_font, sizeof(m_lf), &m_lf);

		if (zoom.num != zoom.denom) { // zoom is not 1:1
			m_lf.lfHeight *= zoom.num;
			m_lf.lfHeight /= zoom.denom;

			m_modified = CreateFontIndirect(&m_lf);
			SelectObject(m_dc, m_modified);
		}
	}

	painter::painter(HDC dc, ratio zoom, HFONT font)
		: painter(dc, zoom, { 0, 0, 0, 0 }, font)
	{
	}

	painter::~painter()
	{
		::SelectObject(m_dc, m_original);
		if (m_modified)
			DeleteObject(m_modified);
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

	void painter::paintImage(const image_ref* img, const pixels& width, const pixels& height)
	{
		auto bmp = reinterpret_cast<Gdiplus::Bitmap*>(img ? img->getNativeHandle() : nullptr);
		if (img && img->getState() != gui::load_state::pixmap_available)
			bmp = nullptr;

		if (!bmp) {
			// TODO: no-image
			return;
		}

		Gdiplus::Graphics gfx{ m_dc };
		gfx.DrawImage(bmp, m_zoom.scaleF(m_origin.x), m_zoom.scaleF(m_origin.y),
			m_zoom.scaleF(width), m_zoom.scaleF(height));
	}

	void painter::paintString(const std::string& text)
	{
		if (text.empty())
			return;

		auto widen = utf::widen(text);
		::TextOut(m_dc, m_zoom.scaleL(m_origin.x), m_zoom.scaleL(m_origin.y), widen.c_str(), widen.length());

#if 0
		SIZE s = {};
		TEXTMETRIC tm = {};
		dc.GetTextExtent(widen.c_str(), widen.length(), &s);
		dc.GetTextMetrics(&tm);
		dc.FillSolidRect(x, y + tm.tmAscent, s.cx - 1, 1, 0x3333FF);
#endif
	}

	size painter::measureString(const std::string& text)
	{
		auto line = utf::widen(text);
		SIZE s = {};
		if (::GetTextExtentPoint32(m_dc, line.c_str(), line.length(), &s))
			return{ (size_t)s.cx, (size_t)s.cy };
		return{};
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

	bool painter::visible(node* node) const
	{
		auto br = m_origin + node->getSize();

		RECT r = { m_zoom.scaleI(m_origin.x), m_zoom.scaleI(m_origin.y),
			m_zoom.scaleI(br.x), m_zoom.scaleI(br.y) };
		RECT test;

		return !!IntersectRect(&test, &r, &m_clip);
	}

	gui::style_handle painter::applyStyle(node* node)
	{
		auto mem = std::make_unique<style_save>();
		mem->apply(this, node);
		return mem.release();
	}

	void painter::restoreStyle(gui::style_handle saved)
	{
		std::unique_ptr<style_save> mem{ (style_save*)saved };
		if (mem)
			mem->restore();
	}

	static void FillSolidRect(HDC dc, LPCRECT lpRect, COLORREF clr)
	{
		COLORREF clrOld = ::SetBkColor(dc, clr);
		::ExtTextOut(dc, 0, 0, ETO_OPAQUE, lpRect, NULL, 0, NULL);
		::SetBkColor(dc, clrOld);
	}

	void painter::drawBackground(gui::node* node, colorref)
	{
		auto style = node->calculatedStyle();
		auto& ref = *style;
		if (!ref.has(styles::prop_background))
			return;

		auto pt = node->getAbsolutePos();
		auto sz = node->getSize();
		RECT r{ m_zoom.scaleI(pt.x) - 1, m_zoom.scaleI(pt.y) - 1,
			m_zoom.scaleI(pt.x + sz.width) + 1, m_zoom.scaleI(pt.y + sz.height) + 1 };
		FillSolidRect(m_dc, &r, ref.get(styles::prop_background));
	}

	struct Border {
		pixels width;
		line_style style;
		colorref color;

		Border(const styles::rule_storage& styles,
			styles::length_prop width_prop,
			styles::border_style_prop style_prop,
			styles::color_prop color_prop)
			: width(0)
			, style(line_style::none)
			, color(0x000000)
		{
			if (styles.has(width_prop)) {
				auto u = styles.get(width_prop);
				ASSERT(u.which() == styles::length_u::first_type);
				width = u.first().value();
			}

			if (styles.has(style_prop))
				style = styles.get(style_prop);

			if (styles.has(color_prop))
				color = styles.get(color_prop);

			if (width.value() == 0 ||
				style == line_style::none)
			{
				width = pixels();
				style = line_style::none;
			}
		};

		bool present() const { return style != line_style::none; }
	};

	void painter::drawBorder(gui::node* node)
	{
		auto styles = node->calculatedStyle();
#define BORDER_(side) \
Border border_ ## side{*styles, \
	styles::prop_border_ ## side ## _width, \
	styles::prop_border_ ## side ## _style, \
	styles::prop_border_ ## side ## _color};
		MAKE_FOURWAY(BORDER_)
#undef BORDER_

			auto pt = node->getAbsolutePos();
		auto sz = node->getSize();

		if (border_top.present()) {
			auto orig = pt;
			auto dest = pt + size{ sz.width, border_top.width };

			if (border_right.present())
				dest.x -= border_right.width;

			drawBorder(orig, dest, border_top.style, border_top.color);
		}

		if (border_right.present()) {
			auto orig = pt;
			auto dest = pt + size{ sz.width, sz.height };
			orig.x = dest.x - border_right.width;

			if (border_bottom.present())
				dest.y -= border_bottom.width;

			drawBorder(orig, dest, border_right.style, border_right.color);
		}

		if (border_bottom.present()) {
			auto orig = pt;
			auto dest = pt + size{ sz.width, sz.height };
			orig.y = dest.y - border_bottom.width;

			if (border_left.present())
				orig.x += border_left.width;

			drawBorder(orig, dest, border_bottom.style, border_bottom.color);
		}

		if (border_left.present()) {
			auto orig = pt;
			auto dest = pt + size{ border_left.width, sz.height };

			if (border_top.present())
				orig.y += border_top.width;

			drawBorder(orig, dest, border_left.style, border_left.color);
		}
	}

	gui::painter* painter::getPainter()
	{
		return this;
	}

	COLORREF painter::getColor() const
	{
		return GetTextColor(m_dc);
	}

	const LOGFONT& painter::getFont() const
	{
		return m_lf;
	}

	void painter::setColor(COLORREF color)
	{
		SetTextColor(m_dc, color);
	}

	void painter::setFont(const LOGFONT& lf)
	{
		m_lf = lf;

		if (m_modified)
			DeleteObject(m_modified);

		m_modified = CreateFontIndirect(&m_lf);
		SelectObject(m_dc, m_modified);
	}

	void painter::drawBorder(const gui::point& from, const gui::point& to, line_style /*style*/, COLORREF color)
	{
		RECT r{ m_zoom.scaleI(from.x), m_zoom.scaleI(from.y), m_zoom.scaleI(to.x), m_zoom.scaleI(to.y) };
		FillSolidRect(m_dc, &r, color);
	}
}};
