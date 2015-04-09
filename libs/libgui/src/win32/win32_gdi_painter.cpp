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

	painter::painter(HDC dc, ratio zoom, ratio device, const RECT& clip, const pixels& fontSize, const std::string& fontFamily)
		: base::painter(zoom, device, fontSize, fontFamily)
		, m_dc{ dc }
		, m_modified{ nullptr }
		, m_original{ nullptr }
		, m_clip{ clip.left, clip.top, clip.right, clip.bottom }
	{
		selectFont(fontSize, fontFamily, FW_NORMAL, false, false);
	}

	painter::painter(HDC dc, ratio zoom, ratio device, const pixels& fontSize, const std::string& fontFamily)
		: painter(dc, zoom, device, { 0, 0, 0, 0 }, fontSize, fontFamily)
	{
	}

	painter::~painter()
	{
		if (m_original)
			::SelectObject(m_dc, m_original);
		if (m_modified)
			DeleteObject(m_modified);
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
		gfx.DrawImage(bmp, zoom().scaleF(origin().x), zoom().scaleF(origin().y),
			zoom().scaleF(width), zoom().scaleF(height));
	}

	void painter::paintString(const std::string& text)
	{
		if (text.empty())
			return;

		auto widen = utf::widen(text);
		::TextOut(m_dc, zoom().scaleL(origin().x), zoom().scaleL(origin().y), widen.c_str(), widen.length());

#if 0
		SIZE s = {};
		TEXTMETRIC tm = {};
		dc.GetTextExtent(widen.c_str(), widen.length(), &s);
		dc.GetTextMetrics(&tm);
		dc.FillSolidRect(x, y + tm.tmAscent, s.cx - 1, 1, 0x3333FF);
#endif
	}

	struct RectF {
		point pt;
		size sz;
	};

	static void FillSolidRect(HDC dc, const RectF& rect, const ratio& zoom, COLORREF clr)
	{
		RECT r{ zoom.scaleL(rect.pt.x), zoom.scaleL(rect.pt.y),
			zoom.scaleL(rect.pt.x + rect.sz.width), zoom.scaleL(rect.pt.y + rect.sz.height) };
		COLORREF clrOld = ::SetBkColor(dc, clr);
		::ExtTextOut(dc, 0, 0, ETO_OPAQUE, &r, NULL, 0, NULL);
		::SetBkColor(dc, clrOld);
	}

	size painter::measureString(const std::string& text)
	{
		auto line = utf::widen(text);
		SIZE s = {};
		if (::GetTextExtentPoint32(m_dc, line.c_str(), line.length(), &s))
			return{ zoom().invert(s.cx), zoom().invert(s.cy) };
		return{};
	}

	void painter::fillRectangle(colorref color, const point& pt, const size& size)
	{
		FillSolidRect(m_dc, { origin() + pt, size }, zoom(), color);
	}

	void painter::drawBorder(line_style /*style*/, colorref color, const gui::point& pt, const gui::size& size)
	{
		FillSolidRect(m_dc, { origin() + pt, size }, zoom(), color);
	}

	static int gui2win32(weight w)
	{
		ASSERT(w != weight::bolder);
		ASSERT(w != weight::lighter);
		switch (w) {
		case weight::normal: return FW_NORMAL;
		case weight::bold: return FW_BOLD;
		default:
			break;
		}

		return (int)w;
	}

	void painter::setFont(const pixels& fontSize, const std::string& fontFamily, gui::weight weight, bool italic, bool underline)
	{
		selectFont(fontSize, fontFamily, gui2win32(weight), italic, underline);
	}

	void painter::setColor(colorref color)
	{
		SetTextColor(m_dc, color);
	}

	colorref painter::getColor() const
	{
		return GetTextColor(m_dc);
	}

	void painter::selectFont(const pixels& fontSize, const std::string& fontFamily, int weight, bool italic, bool underline)
	{
		if (m_modified)
			DeleteObject(m_modified);

		m_modified = ::CreateFont(-zoom().scaleI(fontSize), 0, 0, 0, weight, italic, underline,
			FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, utf::widen(fontFamily).c_str());

		auto tmp = (HFONT)SelectObject(m_dc, m_modified);
		if (!m_original)
			m_original = tmp;
	}
}};
